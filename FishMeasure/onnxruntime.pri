# ONNX Runtime 配置
ONNXRUNTIME_ROOT = D:/QPRO/onnxruntime-win-x64-1.21.0

INCLUDEPATH += $$ONNXRUNTIME_ROOT/include

win32 {
    LIBS += -L$$ONNXRUNTIME_ROOT/lib \
            -lonnxruntime
}

DEFINES += USE_ONNXRUNTIME
message("ONNX Runtime configured")
