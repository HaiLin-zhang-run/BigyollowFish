#pragma once
#include <QWidget>
#include <QImage>

class CameraPreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit CameraPreviewWidget(QWidget* parent = nullptr);
    
public slots:
    void updateImage(const QImage& img);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    QImage currentImage_;
};
