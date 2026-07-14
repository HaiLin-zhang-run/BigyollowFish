#include "camerapreviewwidget.h"
#include <QPainter>

CameraPreviewWidget::CameraPreviewWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(640, 480);
}

void CameraPreviewWidget::updateImage(const QImage& img) {
    currentImage_ = img;
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
    } else {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "无相机画面");
    }
}
