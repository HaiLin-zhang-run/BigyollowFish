#pragma once
#include <QMainWindow>
#include <QTimer>
#include <QProcess>
#include "../core/recordmanager.h"
#include "../camera/orbbeccamera.h"
#include "../detection/fishdetector.h"
#include "../detection/keypointdetector.h"
#include "../detection/morphocalculator.h"
#include "../serial/scaleserial.h"
#include <QtConcurrent>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class CameraPreviewWidget;
class FishListWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCameraFrame(QImage rgb, cv::Mat bgr, cv::Mat depth);
    void onWeightUpdated(double weight);
    void onScaleStatusChanged(const QString& status);
    void onCameraError(const QString& err);
    
    // UI actions
    void on_btnConnectCamera_clicked();
    void on_btnRefreshCamera_clicked();
    void on_btnSetExposure_clicked();
    void on_btnCapture_clicked();
    void on_btnClose_clicked();
    void on_btnTare_clicked();
    void on_btnZero_clicked();
    void on_btnBrowsePath_clicked();
    void on_btnConnectSerial_clicked();
    void on_btnRefreshSerial_clicked();
    void on_btnCalibrateScale_clicked();
    void on_cbWeightUnit_currentIndexChanged(int index);
    
    void on_btnConfirmFishId_clicked();
    void on_btnModifyFishId_clicked();
    void on_btnUploadImage_clicked();
    
    void onOcrTimerTimeout();
    void onOcrProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void on_chkAutoScan_stateChanged(int state);
    
private:
    Ui::MainWindow *ui;
    
    // Core Modules
    RecordManager recordManager_;
    OrbbecCamera camera_;
    FishDetector fishDetector_;
    KeypointDetector kpDetector_;
    MorphoCalculator morphoCalc_;
    ScaleSerial scaleSerial_;
    
    // Widgets
    CameraPreviewWidget* cameraPreview_  = nullptr;
    FishListWidget*      fishList_       = nullptr;
    
    double currentWeight_ = 0.0;
    bool modelsLoaded_ = false;
    QPushButton* btnUploadImage_ = nullptr;

    QTimer* ocrTimer_ = nullptr;
    QProcess* ocrProcess_ = nullptr;
    bool autoScanEnabled_ = false;

    // 当前帧缓存（拍照时传给次界面做推理）
    cv::Mat currentRawBgr_;
    cv::Mat currentDepth_;
    QImage  currentImage_;
    
    void setupUiCustom();
    void initModules();
};
