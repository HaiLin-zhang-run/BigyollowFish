#pragma once
#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVector3D>
#include <QCheckBox>
#include <QResizeEvent>
#include <opencv2/core.hpp>

struct CameraIntrinsics;

/**
 * @brief 3D点云渲染控件
 * 利用深度图和相机内参计算3D坐标，用OpenGL GL_POINTS渲染
 * 支持鼠标旋转、缩放
 */
class PointCloudViewer : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    Q_OBJECT
public:
    explicit PointCloudViewer(QWidget* parent = nullptr);
    ~PointCloudViewer() override;

    // 上传数据（在主线程调用）
    void setData(const cv::Mat& depthMat,
                 const cv::Mat& colorBgr,
                 float fx, float cy, float cx, float fy,
                 const cv::Rect& roiRect = cv::Rect());

    bool saveToPlyBinary(const QString& filepath) const;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    struct Point3D {
        float x, y, z;
        float r, g, b;     // Real Color
        float nx, ny, nz;
        float jr, jg, jb;  // Jet Color
    };

    std::vector<Point3D> points_;
    
    QMatrix4x4 projection_;
    float yaw_    =   0.0f;
    float pitch_  = -20.0f;
    float zoom_   = -800.0f;
    float panX_   =   0.0f;
    float panY_   =   0.0f;
    float centerX_=   0.0f;
    float centerY_=   0.0f;
    float centerZ_= -800.0f;
    QPoint lastMousePos_;
    
    bool dataReady_ = false;
    bool useRealColor_ = false;
    QCheckBox* cbRealColor_ = nullptr;
    
    // OpenGL buffer
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    
    std::unique_ptr<class QOpenGLShaderProgram> prog_;
    
    void rebuildBuffer();
};
