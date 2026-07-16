#include "pointcloudviewer.h"
#include <QOpenGLShaderProgram>
#include <QPainter>
#include <QFont>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <QDebug>
#include <QFile>
#include <QTextStream>

// ─── GLSL shaders ────────────────────────────────────────────────────────────
static const char* VS_SRC = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
out vec3 fragPos;
out vec3 fragColor;
out vec3 fragNormal;
uniform mat4 mvp;
void main() {
    gl_Position  = mvp * vec4(aPos, 1.0);
    gl_PointSize = 3.5;  // 稍微增大点，让它们连成面
    fragPos      = aPos;
    fragColor    = aColor;
    fragNormal   = aNormal;
}
)";

static const char* FS_SRC = R"(
#version 330 core
in  vec3 fragPos;
in  vec3 fragColor;
in  vec3 fragNormal;
out vec4 outColor;
void main() {
    // 法线归一化
    vec3 norm = normalize(fragNormal);
    
    // 假设光照从相机的左上方照过来 (World Space 中，Z为正指向屏幕外)
    vec3 lightDir = normalize(vec3(0.5, 0.8, 1.0));
    
    // Lambertian 漫反射计算
    float diff = max(dot(norm, lightDir), 0.0);
    
    // 环境光与漫反射光叠加
    vec3 ambient = 0.5 * fragColor;       // 提高环境光，防止暗部死黑
    vec3 diffuse = diff * 0.6 * fragColor;
    
    outColor = vec4(ambient + diffuse, 1.0);
}
)";

// ─── 伪彩色映射函数 (Jet Colormap) ──────────────────────────────────────────
static void getJetColor(float v, float vmin, float vmax, float& r, float& g, float& b) {
    float dv = vmax - vmin;
    if (dv < 1e-5f) { r = g = b = 1.0f; return; }
    float ratio = (v - vmin) / dv; // 0.0 ~ 1.0

    // 近红远蓝：在当前逻辑中，Z是负数(例如 -1000为远，-200为近)
    // 此时 vmax 是近处，vmin 是远处。
    // ratio = 1.0 对应近处，ratio = 0.0 对应远处。
    // 典型的 Jet 是 0=蓝, 1=红，完美对应“远处蓝，近处红”。
    
    float c[3] = {1.0f, 1.0f, 1.0f};
    if (ratio < 0.125f) {
        c[0] = 0.0f;
        c[1] = 0.0f;
        c[2] = 0.5f + 4.0f * ratio;
    } else if (ratio < 0.375f) {
        c[0] = 0.0f;
        c[1] = 4.0f * (ratio - 0.125f);
        c[2] = 1.0f;
    } else if (ratio < 0.625f) {
        c[0] = 4.0f * (ratio - 0.375f);
        c[1] = 1.0f;
        c[2] = 1.0f - 4.0f * (ratio - 0.375f);
    } else if (ratio < 0.875f) {
        c[0] = 1.0f;
        c[1] = 1.0f - 4.0f * (ratio - 0.625f);
        c[2] = 0.0f;
    } else {
        c[0] = 1.0f - 4.0f * (ratio - 0.875f);
        c[1] = 0.0f;
        c[2] = 0.0f;
    }
    r = c[0]; g = c[1]; b = c[2];
}

// ─────────────────────────────────────────────────────────────────────────────
PointCloudViewer::PointCloudViewer(QWidget* parent)
    : QOpenGLWidget(parent) {
    setMinimumSize(400, 200);
}

PointCloudViewer::~PointCloudViewer() {
    makeCurrent();
    if (vao_) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_) { glDeleteBuffers(1, &vbo_); }
    doneCurrent();
}

// ─── setData：深度图+彩色图 → 3D点云并计算法线 ──────────────────────────────────────
void PointCloudViewer::setData(const cv::Mat& depthMat,
                               const cv::Mat& colorBgr,
                               float fx, float fy, float cx, float cy,
                               const cv::Rect& /*roiRect*/) {

    if (depthMat.empty() || colorBgr.empty()) {
        qDebug() << "[PC] setData: empty input";
        return;
    }
    if (depthMat.type() != CV_16U) {
        qDebug() << "[PC] depthMat type is not CV_16U:" << depthMat.type();
        return;
    }

    qDebug() << "[PC] depth:" << depthMat.cols << "x" << depthMat.rows
             << "color:" << colorBgr.cols << "x" << colorBgr.rows
             << "fx=" << fx << "fy=" << fy << "cx=" << cx << "cy=" << cy;

    points_.clear();

    const float scaleX = (colorBgr.cols > 0 && depthMat.cols > 0)
                         ? (float)colorBgr.cols / depthMat.cols : 1.0f;
    const float scaleY = (colorBgr.rows > 0 && depthMat.rows > 0)
                         ? (float)colorBgr.rows / depthMat.rows : 1.0f;

    const int step = 2;  // 采样步长（越小越密，网格感越好，但性能开销稍大）
    points_.reserve((depthMat.rows / step) * (depthMat.cols / step));

    float minZ =  1e9f, maxZ = -1e9f;
    float sumX = 0, sumY = 0, sumZ = 0;
    int   cnt  = 0;

    // 辅助闭包：获取某像素的三维坐标
    auto get3D = [&](int u, int v, float& X, float& Y, float& Z_out) -> bool {
        if (u < 0 || u >= depthMat.cols || v < 0 || v >= depthMat.rows) return false;
        uint16_t dv = depthMat.at<uint16_t>(v, u);
        if (dv < 50 || dv > 6000) return false;
        
        float Z = (float)dv;
        X = (u * scaleX - cx) * Z / fx;
        Y = -(v * scaleY - cy) * Z / fy; // Y 取负
        Z_out = -Z;                      // Z 取负
        return true;
    };

    for (int v = 0; v < depthMat.rows; v += step) {
        for (int u = 0; u < depthMat.cols; u += step) {
            float X, Y, Z;
            if (!get3D(u, v, X, Y, Z)) continue;

            Point3D pt;
            pt.x = X;
            pt.y = Y;
            pt.z = Z;

            // 暂定白色，后面统一染伪彩色
            pt.r = 1.0f;
            pt.g = 1.0f;
            pt.b = 1.0f;

            // 计算法线：取向右和向下的邻居像素点
            float nx = 0.0f, ny = 0.0f, nz = 1.0f; // 默认朝向相机
            float xR, yR, zR, xD, yD, zD;
            
            bool hasR = get3D(u + step, v, xR, yR, zR);
            bool hasD = get3D(u, v + step, xD, yD, zD);
            
            if (hasR && hasD) {
                // 如果深度差异过大（边缘），不计算法线，防止拉伸面
                if (std::abs(zR - Z) < 50.0f && std::abs(zD - Z) < 50.0f) {
                    float v1x = xR - X, v1y = yR - Y, v1z = zR - Z; // Right vector
                    float v2x = xD - X, v2y = yD - Y, v2z = zD - Z; // Down vector
                    
                    // Cross Product: v2 x v1 (因为Y轴翻转了，v2是向下(-Y)，v1是向右(+X))
                    // 这样叉乘出来的法线指向正Z（即朝向屏幕外部）
                    float cx_norm = v2y * v1z - v2z * v1y;
                    float cy_norm = v2z * v1x - v2x * v1z;
                    float cz_norm = v2x * v1y - v2y * v1x;
                    
                    float len = std::sqrt(cx_norm*cx_norm + cy_norm*cy_norm + cz_norm*cz_norm);
                    if (len > 1e-4f) {
                        nx = cx_norm / len;
                        ny = cy_norm / len;
                        nz = cz_norm / len;
                    }
                }
            }
            pt.nx = nx;
            pt.ny = ny;
            pt.nz = nz;

            points_.push_back(pt);

            sumX += pt.x;
            sumY += pt.y;
            sumZ += pt.z;
            minZ = std::min(minZ, pt.z);
            maxZ = std::max(maxZ, pt.z);
            ++cnt;
        }
    }

    qDebug() << "[PC] valid points:" << cnt << "Z range:" << minZ << "~" << maxZ;

    // 二次染色：基于深度(Z)映射伪彩色 (Jet Colormap)
    if (cnt > 0) {
        for (auto& p : points_) {
            getJetColor(p.z, minZ, maxZ, p.r, p.g, p.b);
        }
    }

    if (cnt > 0) {
        centerX_ = sumX / cnt;
        centerY_ = sumY / cnt;
        centerZ_ = sumZ / cnt;
    } else {
        centerX_ = 0; centerY_ = 0; centerZ_ = -800.0f;
    }

    float depth = std::abs(centerZ_);
    zoom_  = -(depth * 0.5f);
    pitch_ = 0.0f;
    yaw_   = 0.0f;
    panX_  = -centerX_;
    panY_  = -centerY_;

    dataReady_ = true;
    makeCurrent();
    rebuildBuffer();
    doneCurrent();
    update();
}

// ─── OpenGL 初始化 ──────────────────────────────────────────────────────────
void PointCloudViewer::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.06f, 0.06f, 0.08f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
}

void PointCloudViewer::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    projection_.setToIdentity();
    projection_.perspective(60.0f, (float)w / std::max(h, 1), 1.0f, 100000.0f);
}

// ─── 渲染 ────────────────────────────────────────────────────────────────────
void PointCloudViewer::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!dataReady_ || points_.empty()) {
        glFinish();
        QPainter p(this);
        p.setPen(QColor(160, 160, 200));
        p.setFont(QFont("Microsoft YaHei", 11));
        p.drawText(rect(), Qt::AlignCenter, "暂无点云数据\n请先打开相机并拍照");
        return;
    }

    if (!prog_) {
        prog_ = std::make_unique<QOpenGLShaderProgram>();
        bool ok = true;
        ok &= prog_->addShaderFromSourceCode(QOpenGLShader::Vertex,   VS_SRC);
        ok &= prog_->addShaderFromSourceCode(QOpenGLShader::Fragment, FS_SRC);
        ok &= prog_->link();
        if (!ok) {
            qWarning() << "[PC] shader error:" << prog_->log();
            prog_.reset();
            return;
        }
    }

    QMatrix4x4 view;
    view.translate(panX_, panY_, zoom_);     
    view.rotate(pitch_, 1.0f, 0.0f, 0.0f); 
    view.rotate(yaw_,   0.0f, 1.0f, 0.0f); 
    view.translate(-centerX_, -centerY_, -centerZ_); 

    QMatrix4x4 mvp = projection_ * view;

    prog_->bind();
    prog_->setUniformValue("mvp", mvp);

    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, (GLsizei)points_.size());
    glBindVertexArray(0);

    prog_->release();

    QPainter p(this);
    p.setPen(QColor(180, 180, 200, 180));
    p.setFont(QFont("Microsoft YaHei", 8));
    p.drawText(8, height() - 8,
               QString("左键旋转 | 右/中键平移 | 滚轮缩放 | 双击重置  [渲染点数: %1 | 3D体积光照开启]")
               .arg(points_.size()));
}

// ─── VBO 更新 ────────────────────────────────────────────────────────────────
void PointCloudViewer::rebuildBuffer() {
    if (points_.empty()) return;
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizei)(points_.size() * sizeof(Point3D)),
                 points_.data(),
                 GL_DYNAMIC_DRAW);
    
    // aPos (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point3D),
                          (void*)offsetof(Point3D, x));
    glEnableVertexAttribArray(0);
    
    // aColor (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point3D),
                          (void*)offsetof(Point3D, r));
    glEnableVertexAttribArray(1);
    
    // aNormal (location = 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Point3D),
                          (void*)offsetof(Point3D, nx));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

// ─── 鼠标交互 ────────────────────────────────────────────────────────────────
void PointCloudViewer::mousePressEvent(QMouseEvent* e) {
    lastMousePos_ = e->pos();
    if (e->type() == QEvent::MouseButtonDblClick) {
        float depth = std::abs(centerZ_);
        zoom_  = -(depth * 0.5f);
        pitch_ = 0.0f;
        yaw_   = 0.0f;
        panX_  = -centerX_;
        panY_  = -centerY_;
        update();
    }
}

void PointCloudViewer::mouseMoveEvent(QMouseEvent* e) {
    QPoint delta = e->pos() - lastMousePos_;
    lastMousePos_ = e->pos();

    if (e->buttons() & Qt::LeftButton) {
        yaw_   += delta.x() * 0.4f;
        pitch_ += delta.y() * 0.4f;
        pitch_  = std::max(-89.0f, std::min(89.0f, pitch_));
        update();
    } else if (e->buttons() & (Qt::RightButton | Qt::MiddleButton)) {
        float speed = std::abs(zoom_) * 0.001f;
        panX_ += delta.x() * speed;
        panY_ -= delta.y() * speed;
        update();
    }
}

void PointCloudViewer::wheelEvent(QWheelEvent* e) {
    float factor = 1.0f + e->angleDelta().y() * 0.001f;
    zoom_ *= factor;
    zoom_ = std::max(-50000.0f, std::min(-10.0f, zoom_));
    update();
}

bool PointCloudViewer::saveToPlyBinary(const QString& filepath) const {
    if (points_.empty()) return false;
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    
    QTextStream headerStream(&file);
    headerStream << "ply\n";
    headerStream << "format binary_little_endian 1.0\n";
    headerStream << "element vertex " << points_.size() << "\n";
    headerStream << "property float x\n";
    headerStream << "property float y\n";
    headerStream << "property float z\n";
    headerStream << "property float red\n";
    headerStream << "property float green\n";
    headerStream << "property float blue\n";
    headerStream << "property float nx\n";
    headerStream << "property float ny\n";
    headerStream << "property float nz\n";
    headerStream << "end_header\n";
    headerStream.flush();
    
    file.write((const char*)points_.data(), points_.size() * sizeof(Point3D));
    file.close();
    return true;
}
