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
    
    // Initialize camera button to disabled until serial is connected
    ui->btnStartCamera->setEnabled(false);
    
    // Connect serial port connected signal
    connect(&scaleSerial_, &ScaleSerial::connected, this, [this](bool ok) {
        ui->btnConnectSerial->setText(ok ? "断开" : "连接");
        ui->btnStartCamera->setEnabled(ok);
        if (!ok && camera_.isOpen()) {
            camera_.close();
            ui->btnStartCamera->setText("打开相机");
        }
    });
}

void MainWindow::initModules()
{
    // Load models
    if (!fishDetector_.load(DETECTION_MODEL_PATH)) {
        qWarning() << "Failed to load detection model:" << DETECTION_MODEL_PATH;
    }
    if (!kpDetector_.load(KEYPOINT_MODEL_PATH)) {
        qWarning() << "Failed to load keypoint model:" << KEYPOINT_MODEL_PATH;
    }
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

void MainWindow::onCameraFrame(QImage rgb, cv::Mat depth)
{
    // 主界面不做推理，只展示原始 RGB，保证帧率流畅
    currentDepth_ = depth.clone();

    // 缓存原始 BGR（拍照时用于推理）
    cv::Mat bgr;
    cv::cvtColor(cv::Mat(rgb.height(), rgb.width(), CV_8UC3,
                         (void*)rgb.constBits(), rgb.bytesPerLine()),
                 bgr, cv::COLOR_RGB2BGR);
    currentRawBgr_ = bgr.clone();
    currentImage_  = rgb;

    cameraPreview_->updateImage(rgb);
}

void MainWindow::onWeightUpdated(double weight) {
    currentWeight_ = weight;
    ui->lblWeight->setText(QString("当前重量: %1 g").arg(weight, 0, 'f', 1));
}

void MainWindow::onScaleStatusChanged(const QString& status) {
    ui->lblScaleStatus->setText(status);
}

void MainWindow::on_btnStartCamera_clicked() {
    ui->btnStartCamera->setEnabled(false);
    ui->btnStartCamera->setText("正在连接...");
    
    QtConcurrent::run([this]() {
        bool success = camera_.open();
        
        QMetaObject::invokeMethod(this, [this, success]() {
            if (success) {
                ui->btnStartCamera->setText("相机已开启");
                camera_.startCapture();
            } else {
                ui->btnStartCamera->setText("打开相机");
                ui->btnStartCamera->setEnabled(true);
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::on_btnStopCamera_clicked() {
    camera_.close();
    ui->btnStartCamera->setText("打开相机");
    ui->btnStartCamera->setEnabled(true);
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
