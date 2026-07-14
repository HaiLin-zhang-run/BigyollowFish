#pragma once
#include <QMainWindow>
#include <QTimer>
#include "../core/recordmanager.h"
#include "../camera/orbbeccamera.h"
#include "../detection/fishdetector.h"
#include "../detection/keypointdetector.h"
#include "../detection/morphocalculator.h"
#include "../serial/scaleserial.h"
#include <QtConcurrent>

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
    void onCameraFrame(QImage rgb, cv::Mat depth);
    void onWeightUpdated(double weight);
    void onScaleStatusChanged(const QString& status);
    void onCameraError(const QString& err);
    
    // UI actions
    void on_btnStartCamera_clicked();
    void on_btnStopCamera_clicked();
    void on_btnCapture_clicked();
    void on_btnClose_clicked();
    void on_btnTare_clicked();
    void on_btnZero_clicked();
    void on_btnBrowsePath_clicked();
    void on_btnConnectSerial_clicked();
    
    void on_btnConfirmFishId_clicked();
    void on_btnModifyFishId_clicked();
    
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

    // 当前帧缓存（拍照时传给次界面做推理）
    cv::Mat currentRawBgr_;
    cv::Mat currentDepth_;
    QImage  currentImage_;
    
    void setupUiCustom();
    void initModules();
};
