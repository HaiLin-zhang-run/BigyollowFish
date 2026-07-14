QT += core gui widgets serialport openglwidgets concurrent

CONFIG += c++17
CONFIG -= depend_includepath
TEMPLATE = app
TARGET = FishMeasure

# 包含依赖配置
include($$PWD/orbbec.pri)
include($$PWD/onnxruntime.pri)

# OpenCV 4.12.0
OPENCV = D:/opencv/build
INCLUDEPATH += $$OPENCV/include
win32 {
    LIBS += -L$$OPENCV/x64/vc16/lib
    CONFIG(debug, debug|release) {
        LIBS += -lopencv_world4120d
    } else {
        LIBS += -lopencv_world4120
    }
}

SOURCES += \
    main.cpp \
    core/fishrecord.cpp \
    core/recordmanager.cpp \
    core/csvexporter.cpp \
    camera/orbbeccamera.cpp \
    camera/depthprocessor.cpp \
    detection/onnxhelper.cpp \
    detection/fishdetector.cpp \
    detection/keypointdetector.cpp \
    detection/morphocalculator.cpp \
    serial/scaleserial.cpp \
    ui/mainwindow.cpp \
    ui/camerapreviewwidget.cpp \
    ui/depthviewwidget.cpp \
    ui/fishlistwidget.cpp \
    ui/measurepanelwidget.cpp \
    ui/pointcloudviewer.cpp \
    ui/capturedetailwindow.cpp

HEADERS += \
    core/fishrecord.h \
    core/recordmanager.h \
    core/csvexporter.h \
    camera/orbbeccamera.h \
    camera/depthprocessor.h \
    detection/onnxhelper.h \
    detection/fishdetector.h \
    detection/keypointdetector.h \
    detection/morphocalculator.h \
    serial/scaleserial.h \
    ui/mainwindow.h \
    ui/camerapreviewwidget.h \
    ui/depthviewwidget.h \
    ui/fishlistwidget.h \
    ui/measurepanelwidget.h \
    ui/pointcloudviewer.h \
    ui/capturedetailwindow.h

FORMS += \
    ui/mainwindow.ui

# 模型文件路径定义
DEFINES += \
    DETECTION_MODEL_PATH=\\\"D:/QPRO/best.onnx\\\" \
    KEYPOINT_MODEL_PATH=\\\"D:/QPRO/fish_keypoint.onnx\\\"

# 输出目录
CONFIG(release, debug|release) {
    DESTDIR = $$PWD/bin/release
}
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/bin/debug
}

# Windows图标
# RC_ICONS = resources/app.ico

RESOURCES += \
    resources/resources.qrc
