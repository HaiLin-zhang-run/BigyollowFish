#include "ui/mainwindow.h"
#include <QApplication>

#include <opencv2/core.hpp>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<cv::Mat>("cv::Mat");
    
    // Set global style if needed
    // a.setStyle("Fusion");
    
    MainWindow w;
    w.show();
    
    return a.exec();
}
