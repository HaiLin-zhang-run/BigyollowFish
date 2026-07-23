#include "fishsegmentor.h"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <QDebug>
#include <algorithm>

bool FishSegmentor::load(const std::string& modelPath) {
    return onnx_.loadModel(modelPath);
}

cv::Mat FishSegmentor::segment(const cv::Mat& bgrImg) {
    if (!isLoaded() || bgrImg.empty()) return cv::Mat();

    int srcW = bgrImg.cols, srcH = bgrImg.rows;
    float scaleX = (float)srcW / INPUT_SIZE;
    float scaleY = (float)srcH / INPUT_SIZE;

    // 简单缩放 (Image -> 640x640)
    auto blob = OnnxHelper::imageToBlob(bgrImg, INPUT_SIZE, INPUT_SIZE);
    std::vector<int64_t> shape = {1, 3, INPUT_SIZE, INPUT_SIZE};
    auto outputs = onnx_.run(blob, shape);

    if (outputs.size() < 2) {
        qWarning("FishSegmentor: ONNX output size < 2");
        return cv::Mat();
    }

    // Output0: [1, 37, 8400] (Boxes & Coeffs)
    auto out0Shape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
    const float* data0 = outputs[0].GetTensorData<float>();

    // Output1: [1, 32, 160, 160] (Mask Protos)
    auto out1Shape = outputs[1].GetTensorTypeAndShapeInfo().GetShape();
    const float* protoData = outputs[1].GetTensorData<float>();

    if (out0Shape.size() != 3 || out1Shape.size() != 4) {
        qWarning("FishSegmentor: Unexpected output shape");
        return cv::Mat();
    }
    
    int features = out0Shape[1]; // 37
    int anchors  = out0Shape[2]; // 8400
    
    int numClasses = features - 4 - 32;
    if (numClasses <= 0) {
        qWarning() << "FishSegmentor: Invalid features size:" << features;
        return cv::Mat();
    }

    // 寻找最佳目标 (最高置信度)
    float maxConf = -1.0f;
    int bestAnchor = -1;
    int bestClass = -1;

    for (int i = 0; i < anchors; ++i) {
        for (int c = 0; c < numClasses; ++c) {
            float conf = data0[(4 + c) * anchors + i];
            if (conf > maxConf) {
                maxConf = conf;
                bestAnchor = i;
                bestClass = c;
            }
        }
    }

    if (maxConf < CONF_THRESH || bestAnchor < 0) {
        return cv::Mat(); // 未找到鱼
    }

    // 获取对应的最佳bbox
    float cx = data0[0 * anchors + bestAnchor];
    float cy = data0[1 * anchors + bestAnchor];
    float w  = data0[2 * anchors + bestAnchor];
    float h  = data0[3 * anchors + bestAnchor];

    // 获取 mask coeffs (32维)
    cv::Mat coeffs(1, 32, CV_32F);
    for (int c = 0; c < 32; ++c) {
        coeffs.at<float>(0, c) = data0[(4 + numClasses + c) * anchors + bestAnchor];
    }

    // 处理 Mask Prototype: 32 x 160 x 160
    int maskH = out1Shape[2]; // 160
    int maskW = out1Shape[3]; // 160
    
    // 将 protoData 转成 32 x (160*160) 的矩阵
    cv::Mat protoMat(32, maskH * maskW, CV_32F, const_cast<float*>(protoData));
    
    // Mask 计算 = coeffs(1x32) * protoMat(32 x 25600) -> 1 x 25600
    cv::Mat maskResult = coeffs * protoMat; 
    
    // reshape 成 160x160
    maskResult = maskResult.reshape(1, maskH); 
    
    // Sigmoid: 1 / (1 + exp(-x)) (手动循环，确保 100% 安全)
    for (int r = 0; r < maskResult.rows; ++r) {
        float* ptr = maskResult.ptr<float>(r);
        for (int c = 0; c < maskResult.cols; ++c) {
            ptr[c] = 1.0f / (1.0f + std::exp(-ptr[c]));
        }
    }
    
    // Resize 到原图大小
    cv::Mat maskResized;
    cv::resize(maskResult, maskResized, cv::Size(srcW, srcH));
    
    // 根据阈值二值化 (CV_8UC1)
    cv::Mat finalMask(srcH, srcW, CV_8UC1, cv::Scalar(0));
    
    // 同时考虑 bbox：bbox 外的 mask 我们不要
    int left = std::max(0, (int)((cx - w / 2) * scaleX));
    int top  = std::max(0, (int)((cy - h / 2) * scaleY));
    int right = std::min(srcW, (int)((cx + w / 2) * scaleX));
    int bottom = std::min(srcH, (int)((cy + h / 2) * scaleY));

    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            if (maskResized.at<float>(y, x) > 0.5f) {
                finalMask.at<uchar>(y, x) = 255;
            }
        }
    }
    
    return finalMask;
}
