# OrbbecSDK v2 配置
# SDK源码路径（CMake编译后的输出目录）
ORBBEC_SDK_ROOT = D:/QPRO/OrbbecSDK_v2-main/OrbbecSDK_v2-main

# 头文件
INCLUDEPATH += $$ORBBEC_SDK_ROOT/include

# 编译后的库（CMake build输出）
ORBBEC_BUILD = D:/QPRO/OrbbecSDK_v2-main/build_release

INCLUDEPATH += $$ORBBEC_SDK_ROOT/include \
               $$ORBBEC_BUILD/src/generated

win32 {
    CONFIG(release, debug|release) {
        LIBS += -L$$ORBBEC_BUILD/win_x64/lib \
                -lOrbbecSDK
    }
    CONFIG(debug, debug|release) {
        LIBS += -L$$ORBBEC_BUILD/win_x64/lib \
                -lOrbbecSDK
    }
}

DEFINES += USE_ORBBEC_SDK
message("OrbbecSDK v2 configured")
