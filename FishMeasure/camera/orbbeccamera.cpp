#include "orbbeccamera.h"
#include <QDebug>
#include <libobsensor/ObSensor.hpp>
#include <opencv2/imgproc.hpp>

OrbbecCamera::OrbbecCamera(QObject* parent) : QObject(parent) {}

OrbbecCamera::~OrbbecCamera() {
    close();
}

int OrbbecCamera::getExposure() {
    if (!pipeline_) return -1;
    try {
        auto dev = pipeline_->getDevice();
        if (dev && dev->isPropertySupported(OB_PROP_COLOR_EXPOSURE_INT, OB_PERMISSION_READ)) {
            return dev->getIntProperty(OB_PROP_COLOR_EXPOSURE_INT);
        }
    } catch (...) {}
    return -1;
}

bool OrbbecCamera::setExposure(int value) {
    if (!pipeline_) return false;
    try {
        auto dev = pipeline_->getDevice();
        if (dev && dev->isPropertySupported(OB_PROP_COLOR_EXPOSURE_INT, OB_PERMISSION_WRITE)) {
            if (dev->isPropertySupported(OB_PROP_COLOR_AUTO_EXPOSURE_BOOL, OB_PERMISSION_WRITE)) {
                dev->setBoolProperty(OB_PROP_COLOR_AUTO_EXPOSURE_BOOL, false);
            }
            dev->setIntProperty(OB_PROP_COLOR_EXPOSURE_INT, value);
            return true;
        }
    } catch (...) {}
    return false;
}

bool OrbbecCamera::open() {
    width_ = 0;
    height_ = 0;
    try {
        qDebug() << "--- Camera Open Started ---";
        
        // 先检查设备列表，避免在无设备时实例化 Pipeline 导致底层卡死或抛异常
        ob::Context ctx;
        auto devList = ctx.queryDeviceList();
        if (devList->deviceCount() == 0) {
            qWarning() << "No Orbbec device found!";
            emit errorOccurred("未检测到相机设备，请检查物理连接！");
            emit cameraOpened(false);
            return false;
        }

        qDebug() << "Creating pipeline...";
        pipeline_ = std::make_shared<ob::Pipeline>();
        
        qDebug() << "Creating config...";
        auto config = std::make_shared<ob::Config>();
        
        auto colorProfiles = pipeline_->getStreamProfileList(OB_SENSOR_COLOR);
        std::shared_ptr<ob::VideoStreamProfile> colorProfile = nullptr;
        if (colorProfiles) {
            // Orbbec SDK 会在找不到 profile 时抛出异常，因此必须用 try-catch 包裹
            try { colorProfile = colorProfiles->getVideoStreamProfile(3840, 2160, OB_FORMAT_RGB, 30); } catch (...) {}
            if (!colorProfile) {
                try { colorProfile = colorProfiles->getVideoStreamProfile(2560, 1440, OB_FORMAT_RGB, 30); } catch (...) {}
            }
            if (!colorProfile) {
                try { colorProfile = colorProfiles->getVideoStreamProfile(1920, 1080, OB_FORMAT_RGB, 30); } catch (...) {}
            }
            if (!colorProfile) {
                try { colorProfile = colorProfiles->getVideoStreamProfile(1280, 720, OB_FORMAT_RGB, 30); } catch (...) {}
            }
            if (!colorProfile) {
                try { colorProfile = colorProfiles->getVideoStreamProfile(640, 480, OB_FORMAT_RGB, 30); } catch (...) {}
            }
            if (!colorProfile) {
                try { colorProfile = colorProfiles->getVideoStreamProfile(0, 0, OB_FORMAT_RGB, 0); } catch (...) {}
            }
            if (!colorProfile) {
                colorProfile = colorProfiles->getProfile(0)->as<ob::VideoStreamProfile>();
            }
            if (colorProfile) {
                config->enableStream(colorProfile);
                width_ = colorProfile->width();
                height_ = colorProfile->height();
            }
        }

        auto depthProfiles = pipeline_->getStreamProfileList(OB_SENSOR_DEPTH);
        std::shared_ptr<ob::VideoStreamProfile> depthProfile = nullptr;
        if (depthProfiles) {
            try { depthProfile = depthProfiles->getVideoStreamProfile(640, 480, OB_FORMAT_Y16, 30); } catch (...) {}
            if (!depthProfile) {
                try { depthProfile = depthProfiles->getVideoStreamProfile(0, 0, OB_FORMAT_Y16, 30); } catch (...) {}
            }
            if (!depthProfile) {
                try { depthProfile = depthProfiles->getVideoStreamProfile(0, 0, OB_FORMAT_Y16, 0); } catch (...) {}
            }
            if (!depthProfile) {
                depthProfile = depthProfiles->getProfile(0)->as<ob::VideoStreamProfile>();
            }
            if (depthProfile) config->enableStream(depthProfile);
        }

        qDebug() << "Setting align mode...";
        // 设置深度对齐到彩色（硬件对齐）
        config->setAlignMode(ALIGN_D2C_HW_MODE);

        qDebug() << "Starting pipeline...";
        pipeline_->start(config);

        qDebug() << "Reading camera parameters...";
        auto param = pipeline_->getCameraParam();
        intrinsics_.fx = param.rgbIntrinsic.fx;
        intrinsics_.fy = param.rgbIntrinsic.fy;
        intrinsics_.cx = param.rgbIntrinsic.cx;
        intrinsics_.cy = param.rgbIntrinsic.cy;

        qDebug() << "Camera fully started!";
        running_ = true;
        emit cameraOpened(true);
        return true;
    } catch (const ob::Error& e) {
        qWarning() << "ob::Error exception:" << e.getMessage();
        emit errorOccurred(QString("相机打开失败: %1").arg(e.getMessage()));
        emit cameraOpened(false);
        return false;
    } catch (const std::exception& e) {
        qWarning() << "std::exception:" << e.what();
        emit errorOccurred(QString("标准异常: %1").arg(e.what()));
        emit cameraOpened(false);
        return false;
    } catch (...) {
        qWarning() << "Unknown exception caught!";
        emit errorOccurred(QString("发生未知崩溃异常！"));
        emit cameraOpened(false);
        return false;
    }
}

void OrbbecCamera::close() {
    stopCapture();
    if (pipeline_) {
        try { pipeline_->stop(); } catch (...) {}
        pipeline_.reset();
    }
    running_ = false;
}

void OrbbecCamera::startCapture() {
    if (!running_) return;

    thread_ = QThread::create([this]() {
        captureLoop();
    });
    thread_->start(QThread::HighPriority);
}

void OrbbecCamera::stopCapture() {
    running_ = false;
    if (thread_) {
        thread_->wait(2000);
        delete thread_;
        thread_ = nullptr;
    }
}

void OrbbecCamera::captureLoop() {
    qDebug() << "Capture loop started.";
    int frameCount = 0;
    while (running_) {
        try {
            auto frameSet = pipeline_->waitForFrameset(100);
            if (!frameSet) continue;
            
            auto colorFrame = frameSet->getFrame(OB_FRAME_COLOR);
            auto depthFrame = frameSet->getFrame(OB_FRAME_DEPTH);
            if (!colorFrame || !depthFrame) continue;

            auto vColorFrame = colorFrame->as<ob::VideoFrame>();
            int w = (int)vColorFrame->getWidth();
            int h = (int)vColorFrame->getHeight();
            
            // 防御性检查：确保数据大小足够 RGB888
            if (vColorFrame->getDataSize() < w * h * 3) {
                qDebug() << "Color frame format mismatch or data too small! Format:" << vColorFrame->format();
                continue; // 丢弃这帧
            }
            
            const uint8_t* data = (const uint8_t*)vColorFrame->getData();
            QImage rgb(data, w, h, QImage::Format_RGB888);
            rgb = rgb.copy(); 

            // 后台线程直接转换BGR，避免阻塞主界面
            cv::Mat tmp(h, w, CV_8UC3, (void*)data);
            cv::Mat bgr;
            cv::cvtColor(tmp, bgr, cv::COLOR_RGB2BGR);
            bgr = bgr.clone(); // 保证数据独立

            auto dFrame = depthFrame->as<ob::DepthFrame>();
            if (!dFrame) {
                qDebug() << "Failed to cast to DepthFrame";
                continue;
            }
            int dw = (int)dFrame->getWidth();
            int dh = (int)dFrame->getHeight();
            const uint16_t* depthData = (const uint16_t*)dFrame->getData();
            float scale = dFrame->getValueScale();

            cv::Mat depthMat(dh, dw, CV_16U, const_cast<uint16_t*>(depthData));
            depthMat = depthMat.clone(); 
            if (std::abs(scale - 1.0f) > 0.01f)
                depthMat.convertTo(depthMat, CV_16U, scale);

            if (frameCount % 30 == 0) qDebug() << "Emitting frame..." << frameCount;
            emit frameReady(rgb, bgr, depthMat);
            frameCount++;

        } catch (const ob::Error& e) {
            emit errorOccurred(QString("帧获取错误: %1").arg(e.getMessage()));
        } catch (...) {
            qDebug() << "CRASH inside capture loop!";
        }
    }
    qDebug() << "Capture loop ended.";
}
