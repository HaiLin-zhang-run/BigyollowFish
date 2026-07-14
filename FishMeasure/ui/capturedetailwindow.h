#pragma once
#include <QDialog>
#include <QLabel>
#include <QImage>
#include <QPushButton>
#include <QSplitter>
#include <QProgressBar>
#include <opencv2/core.hpp>
#include "../core/fishrecord.h"
#include "../detection/fishdetector.h"
#include "../detection/keypointdetector.h"
#include "../detection/morphocalculator.h"

class MeasurePanelWidget;
class PointCloudViewer;

/**
 * @brief 拍照结果展示窗口（次界面）
 *
 * 布局：
 *   左侧  ─ 三段截图（鱼头/鱼体/鱼尾）
 *   中间  ─ 上半部分：检测结果图（弹窗内部后台推理）
 *            下半部分：3D 点云（可旋转/缩放）
 *   右侧  ─ 表型数据面板 + 右下角"保存数据信息"按钮
 */
class CaptureDetailWindow : public QDialog {
    Q_OBJECT
public:
    explicit CaptureDetailWindow(QWidget* parent = nullptr);

    /**
     * @brief 传入原始 BGR 图和深度图，窗口内部在后台线程推理
     * @note  必须在 show() 之后调用，以确保 GL 上下文已初始化
     */
    void startDetection(const cv::Mat& rawBgr,
                        const cv::Mat& depthMat,
                        double weight,
                        FishDetector& fishDetector,
                        KeypointDetector& kpDetector,
                        MorphoCalculator& morphoCalc,
                        float fx, float fy, float cx, float cy);

    void setSaveInfo(const QString& dirPath, const QString& fishId);

private slots:
    void onDetectionDone(cv::Mat annotatedBgr, cv::Mat rawBgr, cv::Mat depthMat,
                         FishMorphology morpho, FishKeypoints kps,
                         float fx, float fy, float cx, float cy, bool hasFish);
    void onSaveClicked();
    void onCancelClicked();

signals:
    // 内部用信号，后台完成后触发
    void detectionFinished(cv::Mat annotatedBgr, cv::Mat rawBgr, cv::Mat depthMat,
                           FishMorphology morpho, FishKeypoints kps,
                           float fx, float fy, float cx, float cy, bool hasFish);

private:
    // 左侧 3 张截图
    QLabel* lblHead_;
    QLabel* lblBody_;
    QLabel* lblTail_;

    // 中间上：检测结果图
    QLabel* lblDetection_;

    // 中间下：3D 点云
    PointCloudViewer* cloudViewer_;

    // 右侧数据面板
    MeasurePanelWidget* measurePanel_;

    // 保存和取消按钮
    QPushButton* btnSave_;
    QPushButton* btnCancel_;

    // 保存路径和鱼编号
    QString saveDir_;
    QString fishId_;

    // 保存用数据
    FishMorphology  morpho_;
    FishKeypoints   keypoints_;

    // 辅助：按关键点范围裁剪图像
    QPixmap cropRegion(const cv::Mat& bgr, const FishKeypoints& kps,
                       int startPt, int endPt, int padding = 20);

    // 辅助：设置截图 label 的图
    void setSection(QLabel* lbl, const QPixmap& px, const QSize& hint);
};
