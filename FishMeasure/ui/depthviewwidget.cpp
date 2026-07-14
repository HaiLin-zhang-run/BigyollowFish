#include "depthviewwidget.h"
#include <QPainter>

DepthViewWidget::DepthViewWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 240);
}

void DepthViewWidget::updateImage(const QImage& img) {
    currentImage_ = img;
    update();
}

void DepthViewWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    
    if (!currentImage_.isNull()) {
        QImage scaled = currentImage_.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height()) / 2;
        painter.drawImage(x, y, scaled);
    } else {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "无深度图");
    }
}
