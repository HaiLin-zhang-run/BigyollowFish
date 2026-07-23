#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "camerapreviewwidget.h"
#include "fishlistwidget.h"
#include "capturedetailwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QDebug>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <QDir>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUiCustom();
    initModules();
}

MainWindow::~MainWindow()
{
    camera_.close();
    delete ui;
}

void MainWindow::setupUiCustom()
{
    // Setup Left Panel
    fishList_ = new FishListWidget(&recordManager_, this);
    ui->leftLayout->insertWidget(1, fishList_);
    
    // Setup Center Panel - Only RGB preview
    cameraPreview_ = new CameraPreviewWidget(this);
    ui->centerLayout->addWidget(cameraPreview_);
    
    // Connect history selection
    connect(fishList_, &FishListWidget::recordSelected, this, [this](int index) {
        FishRecord* record = recordManager_.getRecord(index);
        if (!record) return;
        
        auto* dlg = new CaptureDetailWindow(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show(); // Must show before loadRecord to initialize OpenGL context
        dlg->loadRecord(*record);
    });
    
    // Signals
    connect(&camera_, &OrbbecCamera::frameReady, this, &MainWindow::onCameraFrame);
    connect(&camera_, &OrbbecCamera::errorOccurred, this, &MainWindow::onCameraError);
    connect(&scaleSerial_, &ScaleSerial::weightUpdated, this, &MainWindow::onWeightUpdated);
    connect(&scaleSerial_, &ScaleSerial::statusChanged, this, &MainWindow::onScaleStatusChanged);
    
    // Initialize default save path
    ui->editSavePath->setText("D:/QPRO/fish_records.csv");
    
    // Initialize serial ports
    ui->cbSerialPort->addItems(ScaleSerial::availablePorts());
    
    // Connect serial port connected signal
    connect(&scaleSerial_, &ScaleSerial::connected, this, [this](bool ok) {
        ui->btnConnectSerial->setText(ok ? "断开" : "连接");
    });

    // 动态添加一个“上传图片”按钮
    btnUploadImage_ = new QPushButton("上传本地图片进行检测", this);
    btnUploadImage_->setMinimumHeight(40);
    btnUploadImage_->setStyleSheet(
        "QPushButton { font-weight: bold; font-size: 14px; background-color: #2b579a; color: white; border-radius: 4px; }"
        "QPushButton:hover { background-color: #366bc0; }"
        "QPushButton:disabled { background-color: #3f3f46; color: #858585; }"
    );
    btnUploadImage_->setEnabled(false); // 等待模型加载完成
    ui->rightLayout->addWidget(btnUploadImage_);
    connect(btnUploadImage_, &QPushButton::clicked, this, &MainWindow::on_btnUploadImage_clicked);
    
    // 初始化时禁用拍照按钮，等待模型加载
    ui->btnCapture->setEnabled(false);
    ui->btnCapture->setText("模型加载中...");

    ocrTimer_ = new QTimer(this);
    ocrProcess_ = new QProcess(this);
    connect(ocrTimer_, &QTimer::timeout, this, &MainWindow::onOcrTimerTimeout);
    connect(ocrProcess_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onOcrProcessFinished);
}

void MainWindow::initModules()
{
    // 后台异步加载模型，避免阻塞 UI 启动
    (void)QtConcurrent::run([this]() {
        bool detOk = fishDetector_.load(DETECTION_MODEL_PATH);
        if (!detOk) {
            qWarning() << "Failed to load detection model:" << DETECTION_MODEL_PATH;
        }
        bool kpOk = kpDetector_.load(KEYPOINT_MODEL_PATH);
        if (!kpOk) {
            qWarning() << "Failed to load keypoint model:" << KEYPOINT_MODEL_PATH;
        }
        
        bool segOk = fishSegmentor_.load(SEGMENT_MODEL_PATH);
        if (!segOk) {
            qWarning() << "Failed to load segment model:" << SEGMENT_MODEL_PATH;
        }

        QMetaObject::invokeMethod(this, [this, detOk, kpOk, segOk]() {
            modelsLoaded_ = (detOk && kpOk && segOk);
            if (modelsLoaded_) {
                if (btnUploadImage_) btnUploadImage_->setEnabled(true);
                ui->btnCapture->setEnabled(true);
                ui->btnCapture->setText("拍照并测量");
            } else {
                ui->btnCapture->setText("模型加载失败");
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::on_btnConfirmFishId_clicked() {
    ui->editFishId->setEnabled(false);
    ui->btnConfirmFishId->setEnabled(false);
    ui->btnModifyFishId->setEnabled(true);
}

void MainWindow::on_btnModifyFishId_clicked() {
    ui->editFishId->setEnabled(true);
    ui->btnConfirmFishId->setEnabled(true);
    ui->btnModifyFishId->setEnabled(false);
    ui->editFishId->setFocus();
}

void MainWindow::on_btnTare_clicked() {
    scaleSerial_.tare();
    QMessageBox::information(this, "去皮", "去皮指令已发送");
}

void MainWindow::on_btnZero_clicked() {
    scaleSerial_.zero();
    QMessageBox::information(this, "归零", "归零指令已发送");
}

void MainWindow::on_btnBrowsePath_clicked() {
    QString path = QFileDialog::getSaveFileName(this, "选择保存路径", ui->editSavePath->text(), "CSV Files (*.csv)");
    if (!path.isEmpty()) {
        ui->editSavePath->setText(path);
    }
}

void MainWindow::on_btnConnectSerial_clicked() {
    if (scaleSerial_.isConnected()) {
        scaleSerial_.disconnectPort();
        ui->btnConnectSerial->setText("连接");
    } else {
        QString portName = ui->cbSerialPort->currentText();
        if (portName.isEmpty()) {
            QMessageBox::warning(this, "提示", "请选择有效的串口");
            return;
        }
        ui->btnConnectSerial->setText("连接中...");
        scaleSerial_.connectPort(portName);
    }
}

void MainWindow::onCameraFrame(QImage rgb, cv::Mat bgr, cv::Mat depth)
{
    // 主界面不做推理，只展示原始 RGB，保证帧率流畅
    currentDepth_ = depth;
    currentRawBgr_ = bgr;
    currentImage_  = rgb;

    cameraPreview_->updateImage(rgb);
}

void MainWindow::onWeightUpdated(double weight) {
    currentWeight_ = weight;
    QString unit = ui->cbWeightUnit->currentText();
    if (unit == "kg") {
        ui->editWeight->setText(QString("%1").arg(weight / 1000.0, 0, 'f', 3));
    } else {
        ui->editWeight->setText(QString("%1").arg(weight, 0, 'f', 1));
    }
}

void MainWindow::onScaleStatusChanged(const QString& status) {
    ui->lblScaleStatus->setText(status);
}

void MainWindow::on_btnConnectCamera_clicked() {
    if (camera_.isOpen()) {
        ui->btnConnectCamera->setEnabled(false);
        ui->btnConnectCamera->setText("断开中...");
        (void)QtConcurrent::run([this]() {
            camera_.close();
            QMetaObject::invokeMethod(this, [this]() {
                ui->btnConnectCamera->setText("连接");
                ui->btnConnectCamera->setEnabled(true);
            }, Qt::QueuedConnection);
        });
    } else {
        ui->btnConnectCamera->setEnabled(false);
        ui->btnConnectCamera->setText("连接中...");
        (void)QtConcurrent::run([this]() {
            bool success = camera_.open();
            QMetaObject::invokeMethod(this, [this, success]() {
                if (success) {
                    ui->btnConnectCamera->setText("断开");
                    
                    // 获取并显示分辨率和曝光时间
                    ui->editResolution->setText(QString("%1 x %2").arg(camera_.width()).arg(camera_.height()));
                    int exp = camera_.getExposure();
                    if (exp > 0) {
                        ui->editExposureTime->setText(QString::number(exp));
                    } else {
                        ui->editExposureTime->setText("自动/不支持");
                    }
                    
                    camera_.startCapture();
                } else {
                    ui->btnConnectCamera->setText("连接");
                }
                ui->btnConnectCamera->setEnabled(true);
            }, Qt::QueuedConnection);
        });
    }
}

void MainWindow::on_btnRefreshCamera_clicked() {
    ui->cbCameraModel->clear();
    ui->cbCameraModel->addItem("Orbbec Astra 2");
}

void MainWindow::on_btnSetExposure_clicked() {
    bool ok;
    int exp = ui->editExposureTime->text().toInt(&ok);
    if (ok && exp > 0) {
        if (camera_.setExposure(exp)) {
            QMessageBox::information(this, "提示", "曝光时间设置成功");
        } else {
            QMessageBox::warning(this, "提示", "曝光时间设置失败，该型号可能不支持");
        }
    } else {
        QMessageBox::warning(this, "提示", "请输入有效的数字");
    }
}

void MainWindow::on_btnRefreshSerial_clicked() {
    ui->cbSerialPort->clear();
    ui->cbSerialPort->addItems(ScaleSerial::availablePorts());
}

void MainWindow::on_btnCalibrateScale_clicked() {
    bool ok;
    double standardWeight = QInputDialog::getDouble(this, "电子秤校准", 
        "请将已知标准砝码放置在秤盘上，并输入其真实重量（克）:", 
        1000.0, 0.1, 100000.0, 1, &ok);
    if (ok && standardWeight > 0) {
        scaleSerial_.calibrate(standardWeight);
        QMessageBox::information(this, "校准", "校准指令已下发！\n"
                                               "(注: 如果该秤不支持C[重量]格式，可能需要根据具体设备协议修改)");
    }
}

void MainWindow::on_cbWeightUnit_currentIndexChanged(int /*index*/) {
    scaleSerial_.setUnit(ui->cbWeightUnit->currentText());
    onWeightUpdated(currentWeight_);
}

void MainWindow::on_btnCapture_clicked() {
    if (currentRawBgr_.empty()) {
        QMessageBox::warning(this, "提示", "没有可查看的图像，请先打开相机！");
        return;
    }

    auto intr = camera_.colorIntrinsics();
    auto* dlg = new CaptureDetailWindow(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setSaveInfo(ui->editSavePath->text(), ui->editFishId->text());
    
    connect(dlg, &CaptureDetailWindow::recordSaved, this, [this](FishRecord record) {
        record.rgbImage = currentImage_;
        record.depthMat = currentDepth_;
        recordManager_.addRecord(record);
        
        // 测完返回主界面时，自动解锁编号框并重新开始扫码
        on_btnModifyFishId_clicked();
        ui->chkAutoScan->setChecked(true);
    });

    // 先 show()，让 QOpenGLWidget 完成 initializeGL()，再启动后台推理
    dlg->show();
    dlg->startDetection(currentRawBgr_, currentDepth_,
                        currentWeight_,
                        fishDetector_, kpDetector_, morphoCalc_, fishSegmentor_,
                        intr.fx, intr.fy, intr.cx, intr.cy);
}

void MainWindow::on_btnClose_clicked() {
    this->close();
}

void MainWindow::onCameraError(const QString& err) {
    QMessageBox::warning(this, "相机错误", err);
}

void MainWindow::on_btnUploadImage_clicked() {
    QString path = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;

    // 优先用 QImage 读取以支持包含中文的路径，然后转 cv::Mat
    QImage qimg(path);
    if (qimg.isNull()) {
        QMessageBox::warning(this, "错误", "无法读取该图片！");
        return;
    }
    
    qimg = qimg.convertToFormat(QImage::Format_RGB888);
    cv::Mat tmp(qimg.height(), qimg.width(), CV_8UC3, (void*)qimg.constBits(), qimg.bytesPerLine());
    cv::Mat bgr;
    cv::cvtColor(tmp, bgr, cv::COLOR_RGB2BGR);

    // 伪造深度图 (无真实深度信息，3D测量结果将为0，但2D检测仍正常)
    cv::Mat fakeDepth = cv::Mat::zeros(bgr.rows, bgr.cols, CV_16U);

    // 伪造内参
    float fx = bgr.cols;
    float fy = bgr.rows;
    float cx = bgr.cols / 2.0f;
    float cy = bgr.rows / 2.0f;

    auto* dlg = new CaptureDetailWindow(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setSaveInfo(ui->editSavePath->text(), ui->editFishId->text() + "_local");

    connect(dlg, &CaptureDetailWindow::recordSaved, this, [this, qimg, fakeDepth](FishRecord record) {
        record.rgbImage = qimg;
        record.depthMat = fakeDepth;
        recordManager_.addRecord(record);
        
        // 测完返回主界面时，自动解锁编号框并重新开始扫码
        on_btnModifyFishId_clicked();
        ui->chkAutoScan->setChecked(true);
    });

    dlg->show();
    dlg->startDetection(bgr, fakeDepth,
                        0.0,
                        fishDetector_, kpDetector_, morphoCalc_, fishSegmentor_,
                        500.0f, 500.0f, cx, cy);
}

void MainWindow::on_chkAutoScan_stateChanged(int state) {
    autoScanEnabled_ = (state == Qt::Checked);
    cameraPreview_->setShowOcrBox(autoScanEnabled_);
    
    if (autoScanEnabled_) {
        ocrTimer_->start(1000); // 1 second interval
    } else {
        ocrTimer_->stop();
        if (ocrProcess_->state() != QProcess::NotRunning) {
            ocrProcess_->kill();
        }
    }
}

void MainWindow::onOcrTimerTimeout() {
    if (!autoScanEnabled_ || currentRawBgr_.empty() || ocrProcess_->state() != QProcess::NotRunning) {
        return;
    }

    // 截取中心区域 (例如 300x150)
    int cx = currentRawBgr_.cols / 2;
    int cy = currentRawBgr_.rows / 2;
    int rw = 800; // 宽一点适应多个数字
    int rh = 400;
    int rx = std::max(0, cx - rw / 2);
    int ry = std::max(0, cy - rh / 2);
    rw = std::min(rw, currentRawBgr_.cols - rx);
    rh = std::min(rh, currentRawBgr_.rows - ry);

    if (rw <= 0 || rh <= 0) return;

    cv::Mat roi = currentRawBgr_(cv::Rect(rx, ry, rw, rh));
    // 转为灰度图并简单二值化可能会提高tesseract准确率，这里先直接传图
    QString tempPath = QDir::tempPath() + "/ocr_temp.jpg";
    cv::imwrite(tempPath.toStdString(), roi);

    QString tesseractPath = "D:/Tesseract-OCR/tesseract.exe";
    QStringList args;
    args << tempPath << "stdout" << "--psm" << "8" << "-c" << "tessedit_char_whitelist=0123456789";

    ocrProcess_->start(tesseractPath, args);
}

void MainWindow::onOcrProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = QString::fromUtf8(ocrProcess_->readAllStandardOutput()).trimmed();
        // 过滤非数字
        QString digitsOnly;
        for (QChar c : output) {
            if (c.isDigit()) {
                digitsOnly.append(c);
            }
        }
        if (!digitsOnly.isEmpty()) {
            ui->editFishId->setText(digitsOnly);
            
            // 自动检测5位数字并确认
            if (digitsOnly.length() == 5) {
                ui->chkAutoScan->setChecked(false); // 停止自动扫码
                on_btnConfirmFishId_clicked();      // 自动确认
            }
        }
    }
}

