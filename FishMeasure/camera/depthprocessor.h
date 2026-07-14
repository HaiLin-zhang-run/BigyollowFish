#pragma once
#include <opencv2/core.hpp>
#include <QImage>

class DepthProcessor {
public:
    // 将深度图(CV_16U)转为伪彩图用于显示
    static QImage depthToColorMap(const cv::Mat& depthMap, float maxDepth = 2000.0f);
};
