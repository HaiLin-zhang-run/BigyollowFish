#pragma once
#include "onnxhelper.h"
#include <opencv2/core.hpp>

/**
 * @brief best.onnx 鱼体检测器
 *
 * 模型规格（已由Netron确认）：
 *   输入: float32[1,3,640,640]  (RGB, /255.0, CHW)
 *   输出: float32[1,25200,15]
 *         15 = 4(cx,cy,w,h) + 1(conf) + 5×2(5个粗关键点xy)
 *         YOLOv5-Pose, 25200 = 80×80×3 + 40×40×3 + 20×20×3
 */
class FishDetector {
public:
    static constexpr int   INPUT_SIZE   = 640;
    static constexpr float CONF_THRESH  = 0.05f;  // 极度降低阈值，确保即便模型置信度低也能画出框
    static constexpr float NMS_THRESH   = 0.45f;
    static constexpr int   NUM_KPS      = 5;  // 粗关键点数

    struct Detection {
        cv::Rect2f bbox;       // 原图坐标系(像素)
        float      confidence;
        std::array<cv::Point2f, NUM_KPS> coarseKps; // 5个粗关键点(原图坐标)
        bool isValid() const { return confidence > CONF_THRESH; }
    };

    bool load(const std::string& modelPath);
    bool isLoaded() const { return onnx_.isLoaded(); }

    /**
     * @brief 检测图中的鱼体
     * @param bgrImg 输入BGR图像（任意分辨率）
     * @return 最高置信度的检测结果，无鱼时 isValid()=false
     */
    Detection detect(const cv::Mat& bgrImg);

    /**
     * @brief 在图像上绘制检测框
     */
    void drawDetection(cv::Mat& img, const Detection& det) const;

private:
    OnnxHelper onnx_;

    // 解码单行输出 [cx,cy,w,h,conf, kp1x,kp1y,...kp5x,kp5y]
    Detection decodeRow(const float* row,
                        float scaleX, float scaleY) const;

    // NMS
    Detection nms(std::vector<Detection>& dets) const;
};
