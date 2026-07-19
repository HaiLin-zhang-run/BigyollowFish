#include "camerapreviewwidget.h"
#include <QPainter>

CameraPreviewWidget::CameraPreviewWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(640, 480);
}

void CameraPreviewWidget::updateImage(const QImage& img) {
    currentImage_ = img;
    update();
}

void CameraPreviewWidget::setShowOcrBox(bool show) {
    showOcrBox_ = show;
    update();
}

void CameraPreviewWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    
    if (!currentImage_.isNull()) {
        // 等比例缩放居中显示
        QImage scaled = currentImage_.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height()) / 2;
        painter.drawImage(x, y, scaled);
        
        if (showOcrBox_) {
            float scaleX = (float)scaled.width() / currentImage_.width();
            float scaleY = (float)scaled.height() / currentImage_.height();
            
            int origRw = 800;
            int origRh = 400;
            int origRx = currentImage_.width() / 2 - origRw / 2;
            int origRy = currentImage_.height() / 2 - origRh / 2;
            
            int drawX = x + origRx * scaleX;
            int drawY = y + origRy * scaleY;
            int drawW = origRw * scaleX;
            int drawH = origRh * scaleY;
            
            painter.setPen(QPen(Qt::green, 2, Qt::DashLine));
            painter.drawRect(drawX, drawY, drawW, drawH);
            
            painter.setPen(Qt::yellow);
            painter.drawText(drawX, drawY - 5, "请将编号对准此框内");
        }
    } else {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "无相机画面");
    }
}
