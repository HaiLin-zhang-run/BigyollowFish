#pragma once
#include "onnxhelper.h"
#include <opencv2/core.hpp>

class FishSegmentor {
public:
    static constexpr int INPUT_SIZE = 640;
    static constexpr float CONF_THRESH = 0.5f;

    bool load(const std::string& modelPath);
    bool isLoaded() const { return onnx_.isLoaded(); }

    /**
     * @brief 分割出鱼体
     * @param bgrImg 原图
     * @return cv::Mat 返回与原图尺寸一样的二值化Mask (CV_8UC1)。鱼体区域为255，背景为0。如果没有找到鱼，返回空的cv::Mat。
     */
    cv::Mat segment(const cv::Mat& bgrImg);

private:
    OnnxHelper onnx_;
};
