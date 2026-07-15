#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "camerapreviewwidget.h"
#include "fishlistwidget.h"
#include "capturedetailwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <opencv2/imgproc.hpp>

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
        
        QMetaObject::invokeMethod(this, [this, detOk, kpOk]() {
            modelsLoaded_ = (detOk && kpOk);
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
    ui->editWeight->setText(QString("%1").arg(weight, 0, 'f', 1));
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
    QMessageBox::information(this, "校准", "请将已知重量砝码放置在秤盘上进行校准");
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

    // 先 show()，让 QOpenGLWidget 完成 initializeGL()，再启动后台推理
    dlg->show();
    dlg->startDetection(currentRawBgr_, currentDepth_,
                        currentWeight_,
                        fishDetector_, kpDetector_, morphoCalc_,
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

    dlg->show();
    dlg->startDetection(bgr, fakeDepth,
                        0.0, // 本地图片无真实重量
                        fishDetector_, kpDetector_, morphoCalc_,
                        fx, fy, cx, cy);
}
