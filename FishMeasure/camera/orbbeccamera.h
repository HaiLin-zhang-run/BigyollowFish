#pragma once
#include <QObject>
#include <QImage>
#include <QThread>
#include <opencv2/core.hpp>
#include "../detection/morphocalculator.h"

// 前向声明（SDK类型）
namespace ob { class Pipeline; class Config; class FrameSet; }

/**
 * @brief Orbbec Astra2 相机封装
 * 使用 OrbbecSDK v2 C++ API
 * 同步获取 RGB(Color) + Depth 帧，并对齐
 */
class OrbbecCamera : public QObject {
    Q_OBJECT
public:
    explicit OrbbecCamera(QObject* parent = nullptr);
    ~OrbbecCamera();

    bool open();
    void close();
    bool isOpen() const { return running_; }

    // 获取相机内参（成功open后调用）
    MorphoCalculator::Intrinsics colorIntrinsics() const { return intrinsics_; }

    int width() const { return width_; }
    int height() const { return height_; }
    
    int getExposure();
    bool setExposure(int value);

public slots:
    void startCapture();  // 在相机线程中运行
    void stopCapture();

signals:
    // 每帧发出 RGB图 + BGR图 + 深度Mat (CV_16U, 单位mm)
    void frameReady(QImage rgb, cv::Mat bgr, cv::Mat depth);
    void errorOccurred(const QString& msg);
    void cameraOpened(bool ok);

private:
    void captureLoop();  // 帧捕获主循环

    std::shared_ptr<ob::Pipeline> pipeline_;
    QThread*  thread_   = nullptr;
    bool      running_  = false;

    MorphoCalculator::Intrinsics intrinsics_;  // 相机内参缓存
    
    int width_ = 0;
    int height_ = 0;
};
