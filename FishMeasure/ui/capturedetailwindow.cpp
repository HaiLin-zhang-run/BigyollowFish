#include "capturedetailwindow.h"
#include "pointcloudviewer.h"
#include "measurepanelwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFont>
#include <QSplitter>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QtConcurrent>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <algorithm>

#include <QPainter>
#include <QStyleOption>

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: Image Enhancement (Interpolation + Sharpening)
// ─────────────────────────────────────────────────────────────────────────────
static void enhanceImageMat(cv::Mat& img) {
    if (img.empty()) return;
    // 1. 插值放大 (1.5倍，双三次插值)
    cv::resize(img, img, cv::Size(), 1.5, 1.5, cv::INTER_CUBIC);
    // 2. USM锐化 (增强边缘细节)
    cv::Mat blurred;
    cv::GaussianBlur(img, blurred, cv::Size(0, 0), 3.0);
    cv::addWeighted(img, 1.5, blurred, -0.5, 0, img);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: ScaledImageLabel for responsive image rendering
// ─────────────────────────────────────────────────────────────────────────────
class ScaledImageLabel : public QLabel {
public:
    explicit ScaledImageLabel(const QString& text = "") {
        if (!text.isEmpty()) setText(text);
        setAlignment(Qt::AlignCenter);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(50, 50);
        setStyleSheet(
            "QLabel { background:#1e1e1e; border:1px solid #3f3f46;"
            "border-radius:4px; color:#858585; font-size:12px; }");
    }
    void setImage(const QPixmap& p) {
        pix_ = p;
        if (!p.isNull()) setText("");
        update();
    }
protected:
    void paintEvent(QPaintEvent* e) override {
        // Draw background & borders via stylesheet
        QStyleOption opt;
        opt.initFrom(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        if (!pix_.isNull()) {
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            // Scale keeping aspect ratio
            QPixmap scaled = pix_.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int x = (width() - scaled.width()) / 2;
            int y = (height() - scaled.height()) / 2;
            p.drawPixmap(x, y, scaled);
        } else {
            // Draw text if no image
            QLabel::paintEvent(e);
        }
    }
private:
    QPixmap pix_;
};

static ScaledImageLabel* makeDarkLabel(const QString& placeholder = QString())
{
    return new ScaledImageLabel(placeholder);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
CaptureDetailWindow::CaptureDetailWindow(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("鱼体测量详情");
    setMinimumSize(1300, 780);
    resize(1400, 820);

    // ══════════════════════════════════════════════
    //  左侧：三段截图
    // ══════════════════════════════════════════════
    auto* leftBox    = new QGroupBox("鱼体分段图像", this);
    auto* leftLayout = new QVBoxLayout(leftBox);
    leftLayout->setSpacing(6);

    auto makeSectionTitle = [](const QString& t) -> QLabel* {
        auto* l = new QLabel(t);
        l->setStyleSheet("color:#007acc; font-weight:bold; font-size:13px;");
        return l;
    };

    leftLayout->addWidget(makeSectionTitle("鱼头"));
    lblHead_ = makeDarkLabel("检测中...");
    lblHead_->setMinimumHeight(140);
    leftLayout->addWidget(lblHead_);

    leftLayout->addWidget(makeSectionTitle("鱼体"));
    lblBody_ = makeDarkLabel("检测中...");
    lblBody_->setMinimumHeight(140);
    leftLayout->addWidget(lblBody_);

    leftLayout->addWidget(makeSectionTitle("鱼尾"));
    lblTail_ = makeDarkLabel("检测中...");
    lblTail_->setMinimumHeight(140);
    leftLayout->addWidget(lblTail_);

    leftLayout->addStretch();
    leftBox->setFixedWidth(300);

    // ══════════════════════════════════════════════
    //  中间：上半 = 检测图  /  下半 = 3D 点云
    // ══════════════════════════════════════════════

    // -- 上：检测结果图 --
    auto* detBox    = new QGroupBox("检测结果图", this);
    auto* detLayout = new QVBoxLayout(detBox);

    lblDetection_ = makeDarkLabel("⏳ 正在检测，请稍候...");
    lblDetection_->setStyleSheet(
        "QLabel { background:#1e1e1e; border:1px solid #3f3f46;"
        "border-radius:4px; color:#007acc; font-size:15px; font-weight:bold; }");
    detLayout->addWidget(lblDetection_);

    // -- 下：3D 点云 --
    auto* cloudBox    = new QGroupBox("3D 点云视图（拖拽旋转 · 滚轮缩放）", this);
    auto* cloudLayout = new QVBoxLayout(cloudBox);
    cloudViewer_      = new PointCloudViewer(this);
    cloudLayout->addWidget(cloudViewer_);

    // QSplitter 上下分割
    auto* midSplitter = new QSplitter(Qt::Vertical, this);
    midSplitter->addWidget(detBox);
    midSplitter->addWidget(cloudBox);
    midSplitter->setStretchFactor(0, 1);
    midSplitter->setStretchFactor(1, 1);
    midSplitter->setSizes(QList<int>() << 1000 << 1000);
    midSplitter->setStyleSheet("QSplitter::handle { background:#3f3f46; height:4px; margin:2px 0; }");

    auto* midWrapper = new QWidget(this);
    auto* midWLayout = new QVBoxLayout(midWrapper);
    midWLayout->setContentsMargins(0, 0, 0, 0);
    midWLayout->addWidget(midSplitter);

    // ══════════════════════════════════════════════
    //  右侧：数据面板 + 保存按钮
    // ══════════════════════════════════════════════
    auto* rightBox    = new QGroupBox("鱼类表型数据", this);
    auto* rightLayout = new QVBoxLayout(rightBox);

    measurePanel_ = new MeasurePanelWidget(this);
    rightLayout->addWidget(measurePanel_, 1);

    // 右下角：保存数据信息按钮 和 取消按钮
    auto* btnLayout = new QHBoxLayout();

    btnCancel_ = new QPushButton("✖ 取消", this);
    btnCancel_->setMinimumHeight(42);
    btnCancel_->setStyleSheet(R"(
        QPushButton {
            background-color: #3f3f46;
            color: white;
            font-size: 14px;
            font-weight: bold;
            border-radius: 4px;
            border: none;
            padding: 8px 16px;
        }
        QPushButton:hover { background-color: #55555d; }
        QPushButton:pressed { background-color: #2d2d30; }
    )");

    btnSave_ = new QPushButton("💾 保存数据", this);
    btnSave_->setEnabled(false);   // 检测完成前禁用
    btnSave_->setMinimumHeight(42);
    btnSave_->setStyleSheet(R"(
        QPushButton {
            background-color: #007acc;
            color: white;
            font-size: 14px;
            font-weight: bold;
            border-radius: 4px;
            border: none;
            padding: 8px 16px;
        }
        QPushButton:hover {
            background-color: #0098ff;
        }
        QPushButton:pressed {
            background-color: #005a9e;
        }
        QPushButton:disabled {
            background-color: #3f3f46;
            color: #858585;
        }
    )");
    
    btnLayout->addWidget(btnCancel_);
    btnLayout->addWidget(btnSave_);
    rightLayout->addLayout(btnLayout);
    
    rightBox->setFixedWidth(300);

    connect(btnSave_, &QPushButton::clicked, this, &CaptureDetailWindow::onSaveClicked);
    connect(btnCancel_, &QPushButton::clicked, this, &CaptureDetailWindow::onCancelClicked);

    // 连接内部信号（跨线程安全传递 cv::Mat 等）
    connect(this, &CaptureDetailWindow::detectionFinished,
            this, &CaptureDetailWindow::onDetectionDone,
            Qt::QueuedConnection);

    // ══════════════════════════════════════════════
    //  总布局
    // ══════════════════════════════════════════════
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addWidget(leftBox);
    mainLayout->addWidget(midWrapper, 1);
    mainLayout->addWidget(rightBox);

    setStyleSheet(R"(
        QDialog { background:#1e1e1e; }
        QGroupBox {
            color:#d4d4d4;
            font-size:13px;
            font-weight:bold;
            border:1px solid #3f3f46;
            border-radius:4px;
            margin-top:12px;
            padding:8px;
            background:#252526;
        }
        QGroupBox::title {
            subcontrol-origin:margin;
            subcontrol-position:top left;
            left:10px;
            padding:0 6px;
            color:#007acc;
        }
        QLabel { color:#d4d4d4; }
    )");
}

// ─────────────────────────────────────────────────────────────────────────────
//  startDetection – 在后台线程推理，完成后通过 signal 回到主线程更新 UI
// ─────────────────────────────────────────────────────────────────────────────
void CaptureDetailWindow::startDetection(const cv::Mat& rawBgr,
                                          const cv::Mat& depthMat,
                                          double weight,
                                          FishDetector& fishDetector,
                                          KeypointDetector& kpDetector,
                                          MorphoCalculator& morphoCalc,
                                          FishSegmentor& fishSegmentor,
                                          float fx, float fy, float cx, float cy)
{
    // 拷贝数据，确保后台线程安全
    cv::Mat rawBgrCopy   = rawBgr.clone();
    cv::Mat depthCopy    = depthMat.clone();

    // 捕获 detector 指针（主窗口生命期覆盖此对话框）
    FishDetector*    pDet   = &fishDetector;
    KeypointDetector* pKp   = &kpDetector;
    MorphoCalculator* pCalc = &morphoCalc;
    FishSegmentor*   pSeg   = &fishSegmentor;

    (void)QtConcurrent::run([this, rawBgrCopy, depthCopy, weight,
                              pDet, pKp, pCalc, pSeg, fx, fy, cx, cy]() mutable {
        cv::Mat annotated = rawBgrCopy.clone();
        cv::Mat maskMat;
        FishKeypoints kps;
        FishMorphology morpho;
        bool hasFish = false;

        try {
            qDebug() << "[Det] Starting detection, img=" << annotated.cols << "x" << annotated.rows;
            // 注意：经分析发现 best.onnx 实际上是电子秤数字OCR检测模型，而不是鱼体目标检测！
            // fish_keypoint.onnx (HRNet) 原生支持全图输入检测 17 个关键点
            // 因此我们直接用全图范围运行关键点检测，忽略 FishDetector 的错误框
            hasFish = true;
            cv::Rect fullImgRoi(0, 0, annotated.cols, annotated.rows);
            
            qDebug() << "[Det] Running KeypointDetector on full image...";
            kps = pKp->detect(annotated, fullImgRoi);
            hasFish = kps.isValid();
            qDebug() << "[Det] Running FishSegmentor...";
            maskMat = pSeg->segment(rawBgrCopy);
            if (!maskMat.empty()) {
                qDebug() << "[Det] Segmentation mask generated successfully.";
            } else {
                qDebug() << "[Det] Warning: Segmentation mask is empty.";
            }
            
            qDebug() << "[Det] Keypoints done. kp[0] conf=" << kps.conf[0]
                     << "pt=(" << kps.pts[0].x << "," << kps.pts[0].y << ")";
                     
            pCalc->setIntrinsics({fx, fy, cx, cy});
            morpho = pCalc->calculate(kps, depthCopy, annotated.cols, annotated.rows);
            morpho.weight = weight;
            
            // 计算黄蓝值 (b* channel)
            int minX = rawBgrCopy.cols, minY = rawBgrCopy.rows, maxX = 0, maxY = 0;
            bool validKps = false;
            for (int i = 0; i < 17; ++i) {
                if (kps.conf[i] > 0.2f && kps.pts[i].x > 0 && kps.pts[i].y > 0) {
                    minX = std::min(minX, (int)kps.pts[i].x);
                    minY = std::min(minY, (int)kps.pts[i].y);
                    maxX = std::max(maxX, (int)kps.pts[i].x);
                    maxY = std::max(maxY, (int)kps.pts[i].y);
                    validKps = true;
                }
            }
            if (validKps && maxX > minX && maxY > minY) {
                minX = std::max(0, minX - 20);
                minY = std::max(0, minY - 20);
                maxX = std::min(rawBgrCopy.cols - 1, maxX + 20);
                maxY = std::min(rawBgrCopy.rows - 1, maxY + 20);
                
                cv::Rect roi(minX, minY, maxX - minX, maxY - minY);
                cv::Mat fishRoi = rawBgrCopy(roi);
                cv::Mat labMat;
                cv::cvtColor(fishRoi, labMat, cv::COLOR_BGR2Lab);
                std::vector<cv::Mat> labChannels;
                cv::split(labMat, labChannels);
                double minVal, maxVal;
                cv::Point minLoc, maxLoc;
                cv::minMaxLoc(labChannels[2], &minVal, &maxVal, &minLoc, &maxLoc);
                morpho.yellowBlueValue = maxVal - 128.0f;
                
                cv::Point yellowPt(maxLoc.x + minX, maxLoc.y + minY);
                cv::circle(annotated, yellowPt, 8, cv::Scalar(0, 255, 255), -1);
                cv::circle(annotated, yellowPt, 12, cv::Scalar(0, 100, 255), 2);
                cv::putText(annotated, QString("b*:%1").arg(morpho.yellowBlueValue).toStdString(), 
                            yellowPt + cv::Point(15, 0), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 255), 2);
            }

            pKp->drawKeypoints(annotated, kps);
            pKp->drawMeasureLines(annotated, kps);
        } catch (const std::exception& e) {
            qDebug() << "Exception in detection thread:" << e.what();
            hasFish = false;
        } catch (...) {
            qDebug() << "Unknown exception in detection thread";
            hasFish = false;
        }

        // 通过 QueuedConnection 安全回到主线程
        emit detectionFinished(annotated, rawBgrCopy, depthCopy, maskMat,
                               morpho, kps, fx, fy, cx, cy, hasFish);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
//  onDetectionDone – 在主线程更新所有 UI 控件（信号触发）
// ─────────────────────────────────────────────────────────────────────────────
void CaptureDetailWindow::onDetectionDone(cv::Mat annotatedBgr, cv::Mat rawBgr, cv::Mat depthMat, cv::Mat maskMat,
                                          FishMorphology morpho, FishKeypoints kps,
                                          float fx, float fy, float cx, float cy, bool hasFish)
{
    morpho_    = morpho;
    keypoints_ = kps;
    rawBgr_    = rawBgr.clone();

    // 恢复常规边框样式
    lblDetection_->setStyleSheet(
        "QLabel { background:#1e1e1e; border:1px solid #3f3f46;"
        "border-radius:4px; color:#858585; font-size:12px; }");

    // ── 2. 中间上：检测图 ────────────────────────────────────────────────────
    {
        cv::Mat rgb;
        cv::Mat enhanced = annotatedBgr.clone();
        enhanceImageMat(enhanced);
        cv::cvtColor(enhanced, rgb, cv::COLOR_BGR2RGB);
        QImage qimg(rgb.data, rgb.cols, rgb.rows, (int)rgb.step, QImage::Format_RGB888);
        QPixmap pix = QPixmap::fromImage(qimg.copy());

        
        auto* sil = static_cast<ScaledImageLabel*>(lblDetection_);
        if (!pix.isNull()) {
            sil->setImage(pix);
        } else {
            sil->setText("图像转换失败");
        }
    }

    // ── 3. 左侧三段图 ─────────────────────────────────────────────────────────
    if (hasFish) {
        // 鱼头: 吻端(1), 眼(2,3), 鳃盖后缘(15)
        headPix_ = cropRegion(rawBgr, kps, {1, 2, 3, 15});
        setSection(lblHead_, headPix_, QSize(280, 135));
        // 鱼身: 胸鳍(4,14), 背鳍起点(5), 腹部(13), 臀鳍基部前(11), 臀鳍基部后(12)
        bodyPix_ = cropRegion(rawBgr, kps, {4, 5, 11, 12, 13, 14});
        setSection(lblBody_, bodyPix_, QSize(280, 135));
        // 鱼尾: 尾柄背侧(6), 尾柄腹侧(9), 尾鳍基部(8), 尾鳍尖端(7)
        tailPix_ = cropRegion(rawBgr, kps, {6, 7, 8, 9});
        setSection(lblTail_, tailPix_, QSize(280, 135));
    } else {
        lblHead_->setText("未检测到鱼体");
        lblBody_->setText("未检测到鱼体");
        lblTail_->setText("未检测到鱼体");
    }

    // ── 4. 中间下：3D 点云（始终显示完整场景点云）────────────────────────────
    // 传入空 Rect 表示渲染全图
    cloudViewer_->setData(depthMat, rawBgr, fx, fy, cx, cy, cv::Rect(), maskMat);

    // ── 5. 右侧数据面板 ───────────────────────────────────────────────────────
    measurePanel_->updateData(morpho);

    // 解锁保存按钮
    btnSave_->setEnabled(true);
}

// ─────────────────────────────────────────────────────────────────────────────
//  cropRegion
// ─────────────────────────────────────────────────────────────────────────────
QPixmap CaptureDetailWindow::cropRegion(const cv::Mat& bgr,
                                        const FishKeypoints& kps,
                                        const std::vector<int>& ptsIdx, int padding)
{
    int minX = bgr.cols, minY = bgr.rows, maxX = 0, maxY = 0;
    bool anyValid = false;
    for (int idx : ptsIdx) {
        if (kps.conf[idx - 1] < 0.2f) continue;
        const auto& pt = kps.p(idx);
        if (pt.x <= 0 || pt.y <= 0) continue;
        minX = std::min(minX, (int)pt.x);
        minY = std::min(minY, (int)pt.y);
        maxX = std::max(maxX, (int)pt.x);
        maxY = std::max(maxY, (int)pt.y);
        anyValid = true;
    }
    if (!anyValid || maxX <= minX || maxY <= minY) return QPixmap();

    minX = std::max(0, minX - padding);
    minY = std::max(0, minY - padding);
    maxX = std::min(bgr.cols - 1, maxX + padding);
    maxY = std::min(bgr.rows - 1, maxY + padding);
    
    if (maxX <= minX || maxY <= minY) return QPixmap();

    cv::Rect roi(minX, minY, maxX - minX, maxY - minY);
    cv::Mat  crop = bgr(roi).clone();
    enhanceImageMat(crop); // 应用插值放大和锐化
    cv::Mat  rgb;
    cv::cvtColor(crop, rgb, cv::COLOR_BGR2RGB);

    QImage img(rgb.data, rgb.cols, rgb.rows, (int)rgb.step, QImage::Format_RGB888);
    return QPixmap::fromImage(img.copy());
}

// ─────────────────────────────────────────────────────────────────────────────
//  setSection
// ─────────────────────────────────────────────────────────────────────────────
void CaptureDetailWindow::setSection(QLabel* lbl, const QPixmap& px, const QSize& hint)
{
    auto* sil = static_cast<ScaledImageLabel*>(lbl);
    if (!sil) return;
    
    if (px.isNull()) {
        sil->setText("未检测到关键点");
    } else {
        sil->setImage(px);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  setSaveInfo
// ─────────────────────────────────────────────────────────────────────────────
void CaptureDetailWindow::setSaveInfo(const QString& dirPath, const QString& fishId)
{
    saveDir_ = dirPath;
    fishId_ = fishId;
}

// ─────────────────────────────────────────────────────────────────────────────
//  loadRecord
// ─────────────────────────────────────────────────────────────────────────────
void CaptureDetailWindow::loadRecord(const FishRecord& record)
{
    morpho_ = record.morphology;
    keypoints_ = record.keypoints;
    
    // 中间上：检测结果图 -> 改为展示原图
    auto* sil = static_cast<ScaledImageLabel*>(lblDetection_);
    if (!record.rgbImage.isNull()) {
        sil->setImage(QPixmap::fromImage(record.rgbImage));
    } else {
        sil->setText("暂无原始图像");
    }
    
    // 禁用保存按钮（只读模式）
    btnSave_->setEnabled(false);
    btnSave_->setText("历史记录 (只读)");
    
    // 恢复常规边框样式
    lblDetection_->setStyleSheet(
        "QLabel { background:#1e1e1e; border:1px solid #3f3f46;"
        "border-radius:4px; color:#858585; font-size:12px; }");
        
    // 截取左侧三图 (如果存在原始图)
    if (!record.rgbImage.isNull() && keypoints_.isValid()) {
        cv::Mat rawBgr;
        cv::Mat tmp(record.rgbImage.height(), record.rgbImage.width(), CV_8UC3, 
                    (void*)record.rgbImage.constBits(), record.rgbImage.bytesPerLine());
        cv::cvtColor(tmp, rawBgr, cv::COLOR_RGB2BGR);
        
        headPix_ = cropRegion(rawBgr, keypoints_, {1, 2, 3, 15});
        setSection(lblHead_, headPix_, QSize(280, 135));
        bodyPix_ = cropRegion(rawBgr, keypoints_, {4, 5, 11, 12, 13, 14});
        setSection(lblBody_, bodyPix_, QSize(280, 135));
        tailPix_ = cropRegion(rawBgr, keypoints_, {6, 7, 8, 9});
        setSection(lblTail_, tailPix_, QSize(280, 135));
    } else {
        lblHead_->setText("无图像");
        lblBody_->setText("无图像");
        lblTail_->setText("无图像");
    }
    
    // 点云数据 (如果有深度图和 RGB 图)
    if (!record.depthMat.empty() && !record.rgbImage.isNull()) {
        cv::Mat rawBgr;
        cv::Mat tmp(record.rgbImage.height(), record.rgbImage.width(), CV_8UC3, 
                    (void*)record.rgbImage.constBits(), record.rgbImage.bytesPerLine());
        cv::cvtColor(tmp, rawBgr, cv::COLOR_RGB2BGR);
        
        // 我们假设使用默认的内参，如果有真实内参也可以传入
        // 这里只是简单的回放，如果有准确内参效果更好
        cloudViewer_->setData(record.depthMat, rawBgr, 
                              rawBgr.cols, rawBgr.rows, 
                              rawBgr.cols/2.0f, rawBgr.rows/2.0f, cv::Rect());
    }
    
    // 右侧数据面板
    measurePanel_->updateData(morpho_);
}

// ─────────────────────────────────────────────────────────────────────────────
//  onSaveClicked
// ─────────────────────────────────────────────────────────────────────────────
void CaptureDetailWindow::onSaveClicked()
{
    QString dir = saveDir_.isEmpty() ? "D:/QPRO/主目录" : saveDir_;
    QString dateStr = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString idStr = fishId_.isEmpty() ? QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") : fishId_;
    
    QDir d(dir);
    if (!d.exists()) {
        d.mkpath(".");
    }
    
    // 1. 创建日期子文件夹
    if (!d.exists(dateStr)) {
        d.mkpath(dateStr);
    }
    d.cd(dateStr);
    
    // 2. 在日期文件夹中创建以鱼的编号命名的孙文件夹
    if (!d.exists(idStr)) {
        d.mkpath(idStr);
    }
    d.cd(idStr);
    
    QString path = d.filePath(QString("fish_%1.csv").arg(idStr));

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法创建文件：" + path);
        return;
    }
    
    QTextStream ts(&f);
    // 使用系统默认编码 (Windows下通常为 GBK) 写入 CSV，这样 Excel 直接打开就不会乱码了
    ts.setEncoding(QStringConverter::System);
    ts << "指标,数值\n";
    ts << QString("鱼的编号,%1\n").arg(idStr);
    ts << QString("全长(mm),%1\n").arg(morpho_.totalLength,  0, 'f', 1);
    ts << QString("体长(mm),%1\n").arg(morpho_.bodyLength,   0, 'f', 1);
    ts << QString("头长(mm),%1\n").arg(morpho_.headLength,   0, 'f', 1);
    ts << QString("躯干长(mm),%1\n").arg(morpho_.trunkLength, 0, 'f', 1);
    ts << QString("尾长(mm),%1\n").arg(morpho_.tailLength,   0, 'f', 1);
    ts << QString("吻长(mm),%1\n").arg(morpho_.snoutLength,  0, 'f', 1);
    ts << QString("眼长(mm),%1\n").arg(morpho_.eyeLength,    0, 'f', 1);
    ts << QString("眼后头长(mm),%1\n").arg(morpho_.postEyeHeadLength, 0, 'f', 1);
    ts << QString("尾柄长(mm),%1\n").arg(morpho_.caudPedLength,  0, 'f', 1);
    ts << QString("体宽(mm),%1\n").arg(morpho_.bodyHeight,   0, 'f', 1);
    ts << QString("尾柄高(mm),%1\n").arg(morpho_.caudPedHeight,  0, 'f', 1);
    ts << QString("尾鳍长度(mm),%1\n").arg(morpho_.caudalFinLength, 0, 'f', 1);
    ts << QString("臀鳍长度(mm),%1\n").arg(morpho_.analFinLength, 0, 'f', 1);
    ts << QString("厚度(mm),%1\n").arg(morpho_.thickness,    0, 'f', 1);
    ts << QString("体重(g),%1\n").arg(morpho_.weight,       0, 'f', 1);
    ts << QString("胸鳍长度(mm),%1\n").arg(morpho_.pectoralFinLength, 0, 'f', 1);
    ts << QString("眼鳍距(mm),%1\n").arg(morpho_.eyeFinLength, 0, 'f', 1);
    ts << QString("黄蓝值,%1\n").arg(morpho_.yellowBlueValue, 0, 'f', 1);
    f.close();

    // 保存左侧三幅截图
    if (!headPix_.isNull()) headPix_.save(d.filePath(QString("fish_%1_head.jpg").arg(idStr)));
    if (!bodyPix_.isNull()) bodyPix_.save(d.filePath(QString("fish_%1_body.jpg").arg(idStr)));
    if (!tailPix_.isNull()) tailPix_.save(d.filePath(QString("fish_%1_tail.jpg").arg(idStr)));

    // 保存原始 RGB 图片
    if (!rawBgr_.empty()) {
        cv::Mat rgb;
        cv::cvtColor(rawBgr_, rgb, cv::COLOR_BGR2RGB);
        QImage qimg(rgb.data, rgb.cols, rgb.rows, (int)rgb.step, QImage::Format_RGB888);
        qimg.save(d.filePath(QString("fish_%1_rgb.jpg").arg(idStr)));
    }

    cloudViewer_->saveToPlyBinary(d.filePath(QString("fish_%1_cloud.ply").arg(idStr)));

    QMessageBox::information(this, "保存成功", "数据及关联图像、点云已保存到：\n" + d.absolutePath());

    // 构造一条记录并抛出，用于历史列表
    FishRecord record;
    record.id = idStr;
    record.timestamp = QDateTime::currentDateTime();
    record.morphology = morpho_;
    record.keypoints = keypoints_;
    // 从界面上拿到底图，其实我们没有保存原图到类成员，可以考虑仅存已处理图
    // 由于这里只是展示，我们就存入annotatedImage即可，或者如果有原图成员更好
    if (auto* sil = static_cast<ScaledImageLabel*>(lblDetection_)) {
        QPixmap pix = sil->pixmap();
        if (!pix.isNull()) {
            record.annotatedImage = pix.toImage();
        }
    }
    // 发出信号
    emit recordSaved(record);
    
    // 保存成功后关闭次界面，返回主界面
    this->close();
}

// ─────────────────────────────────────────────────────────────────────────────
//  onCancelClicked
// ─────────────────────────────────────────────────────────────────────────────
void CaptureDetailWindow::onCancelClicked()
{
    this->close();
}
