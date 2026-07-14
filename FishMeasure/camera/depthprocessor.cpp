#include "depthprocessor.h"
#include <opencv2/imgproc.hpp>

QImage DepthProcessor::depthToColorMap(const cv::Mat& depthMap, float maxDepth) {
    if (depthMap.empty()) return QImage();

    cv::Mat depth8U;
    // 归一化到 0-255
    depthMap.convertTo(depth8U, CV_8U, 255.0 / maxDepth);
    
    cv::Mat colorMap;
    // 使用 JET 色彩映射表
    cv::applyColorMap(depth8U, colorMap, cv::COLORMAP_JET);
    
    // cv::Mat (BGR) -> QImage (RGB)
    cv::cvtColor(colorMap, colorMap, cv::COLOR_BGR2RGB);
    
    QImage img((const uchar*)colorMap.data, colorMap.cols, colorMap.rows, colorMap.step, QImage::Format_RGB888);
    return img.copy(); // 深拷贝
}
