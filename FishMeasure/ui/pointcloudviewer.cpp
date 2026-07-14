#include "pointcloudviewer.h"
#include <QOpenGLShaderProgram>
#include <QPainter>
#include <QFont>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <QDebug>

// ─── GLSL shaders ────────────────────────────────────────────────────────────
static const char* VS_SRC = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
out vec3 fragColor;
uniform mat4 mvp;
void main() {
    gl_Position  = mvp * vec4(aPos, 1.0);
    gl_PointSize = 2.0;
    fragColor    = aColor;
}
)";

static const char* FS_SRC = R"(
#version 330 core
in  vec3 fragColor;
out vec4 outColor;
void main() {
    outColor = vec4(fragColor, 1.0);
}
)";

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

// ─── setData：深度图+彩色图 → 3D点云 ──────────────────────────────────────
void PointCloudViewer::setData(const cv::Mat& depthMat,
                               const cv::Mat& colorBgr,
                               float fx, float fy, float cx, float cy,
                               const cv::Rect& /*roiRect – 始终渲染全图*/) {

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

    // D2C 对齐后深度图和彩色图分辨率相同，scaleX=scaleY=1
    // 但若不同（软对齐），仍然正确缩放
    const float scaleX = (colorBgr.cols > 0 && depthMat.cols > 0)
                         ? (float)colorBgr.cols / depthMat.cols : 1.0f;
    const float scaleY = (colorBgr.rows > 0 && depthMat.rows > 0)
                         ? (float)colorBgr.rows / depthMat.rows : 1.0f;

    const int step = 3;  // 每隔3像素采样一次
    points_.reserve((depthMat.rows / step) * (depthMat.cols / step));

    // 统计有效深度范围（用百分位避免离群值影响）
    float minZ =  1e9f, maxZ = -1e9f;
    float sumX = 0, sumY = 0, sumZ = 0;
    int   cnt  = 0;

    for (int v = 0; v < depthMat.rows; v += step) {
        for (int u = 0; u < depthMat.cols; u += step) {
            uint16_t dv = depthMat.at<uint16_t>(v, u);
            if (dv < 50 || dv > 6000) continue;

            float Z = (float)dv;  // mm

            float colorU = u * scaleX;
            float colorV = v * scaleY;

            // 针孔模型反投影（内参对应彩色图分辨率）
            float X = (colorU - cx) * Z / fx;
            float Y = (colorV - cy) * Z / fy;

            Point3D pt;
            pt.x =  X;
            pt.y = -Y;   // OpenGL Y轴朝上（翻转）
            pt.z = -Z;   // OpenGL Z轴：相机看向-Z，深度为正所以取负

            // 彩色贴图
            int cU = std::max(0, std::min(colorBgr.cols - 1, (int)colorU));
            int cV = std::max(0, std::min(colorBgr.rows - 1, (int)colorV));
            auto bgr = colorBgr.at<cv::Vec3b>(cV, cU);
            pt.r = bgr[2] / 255.0f;
            pt.g = bgr[1] / 255.0f;
            pt.b = bgr[0] / 255.0f;

            points_.push_back(pt);

            sumX += pt.x;
            sumY += pt.y;
            sumZ += pt.z;
            minZ = std::min(minZ, pt.z);
            maxZ = std::max(maxZ, pt.z);
            ++cnt;
        }
    }

    qDebug() << "[PC] valid points:" << cnt
             << "Z range:" << minZ << "~" << maxZ;

    if (cnt > 0) {
        centerX_ = sumX / cnt;
        centerY_ = sumY / cnt;
        centerZ_ = sumZ / cnt;  // 负值，约 -depthMM
    } else {
        centerX_ = 0; centerY_ = 0; centerZ_ = -800.0f;
    }

    // ── 初始视角：让点云充满视图 ──────────────────────────────────────────
    // 点云在OpenGL坐标: z=centerZ_ (负值), 相机在 z=0 看向 -z 方向
    // 相机需要在 z > centerZ_ 的位置才能看到点云
    // zoom_ 是相机 translate 的 z 偏移量（负值 = 相机向后退）
    // 要看到深度为 -D 的点，camera 应该在 z = -D * factor
    // view.translate(0,0,zoom_) => 点变为 z_view = z_world + zoom_
    // 要让 z_world = centerZ_ 显示在屏幕中心: z_view 需要是合理负值
    // 简单设: zoom_ = 0，相机在原点，点在 z=centerZ_ 处（直接可见）
    // 但这样太近，所以 zoom_ = -|centerZ_| * 0.5f 再退半个深度
    float depth = std::abs(centerZ_);
    zoom_  = -(depth * 0.5f);   // 退后半个深度单位，让点云显示在视野内
    pitch_ = 0.0f;              // 正面朝向
    yaw_   = 0.0f;
    panX_  = -centerX_;        // 水平平移，让点云居中
    panY_  = -centerY_;        // 垂直平移，让点云居中

    qDebug() << "[PC] centerX=" << centerX_ << "centerY=" << centerY_
             << "centerZ=" << centerZ_ << "zoom=" << zoom_;

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
    // 宽视角+大远平面，确保不裁剪
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

    // 懒加载着色器
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
        qDebug() << "[PC] shader linked OK";
    }

    // ── MVP ─────────────────────────────────────────────────────────────────
    // 策略：
    //   1. 旋转（绕点云自身中心）
    //   2. 平移（panX/panY 用于对准）
    //   3. zoom 退后
    // 点在世界坐标 (x, y, z_neg) 中
    // 我们把 世界 -> 相机 的变换分解为:
    //   view = T(panX, panY, zoom) * Rx(pitch) * Ry(yaw) * T(-cx,-cy,-cz中心)
    //
    QMatrix4x4 view;
    view.translate(panX_, panY_, zoom_);     // ③ 退后 + 平移对中
    view.rotate(pitch_, 1.0f, 0.0f, 0.0f); // ② 俯仰
    view.rotate(yaw_,   0.0f, 1.0f, 0.0f); // ① 左右
    view.translate(-centerX_, -centerY_, -centerZ_); // ⓪ 把点云中心移到原点

    QMatrix4x4 mvp = projection_ * view;

    prog_->bind();
    prog_->setUniformValue("mvp", mvp);

    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, (GLsizei)points_.size());
    glBindVertexArray(0);

    prog_->release();

    // HUD
    QPainter p(this);
    p.setPen(QColor(180, 180, 200, 180));
    p.setFont(QFont("Microsoft YaHei", 8));
    p.drawText(8, height() - 8,
               QString("左键旋转 | 右/中键平移 | 滚轮缩放 | 双击重置  [点数: %1]")
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point3D),
                          (void*)offsetof(Point3D, x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point3D),
                          (void*)offsetof(Point3D, r));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

// ─── 鼠标交互 ────────────────────────────────────────────────────────────────
void PointCloudViewer::mousePressEvent(QMouseEvent* e) {
    lastMousePos_ = e->pos();
    if (e->type() == QEvent::MouseButtonDblClick) {
        // 双击重置视角
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
