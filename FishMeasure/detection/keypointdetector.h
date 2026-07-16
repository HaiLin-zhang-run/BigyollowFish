#pragma once
#include "onnxhelper.h"
#include "../core/fishrecord.h"
#include <opencv2/core.hpp>

/**
 * @brief fish_keypoint.onnx 关键点检测器
 *
 * 模型规格（已由Netron确认）：
 *   框架: PaddlePaddle (HRNet风格)
 *   输入: float32[B,3,H,W]  动态尺寸，实测确定
 *   输出:
 *     ① bilinear_interp_v2_1.tmp_0  [B,17,H',W']  热力图
 *     ② top_k_v2_0.tmp_0            [1,17,30]     前30候选分数
 *     ③ top_k_v2_0.tmp_1            [1,17,30]     前30候选flat索引
 *
 *   → 取top_k[0]即最高置信度位置，除以热图宽高得到归一化坐标，
 *     再映射回原图坐标系
 */
class KeypointDetector {
public:
    // 默认输入尺寸（可在运行时自动检测或手动指定）
    static constexpr int DEFAULT_INPUT_W = 640;
    static constexpr int DEFAULT_INPUT_H = 640;

    bool load(const std::string& modelPath);
    bool isLoaded() const { return onnx_.isLoaded(); }

    /**
     * @brief 检测关键点
     * @param bgrImg  原始BGR图像（全图）
     * @param roi     鱼体检测框（来自FishDetector）
     * @return FishKeypoints：17个关键点（原图坐标系）
     */
    FishKeypoints detect(const cv::Mat& bgrImg,
                         const cv::Rect& roi);

    /**
     * @brief 在图上绘制关键点和测量线
     */
    void drawKeypoints(cv::Mat& img,
                       const FishKeypoints& kps) const;
    void drawMeasureLines(cv::Mat& img,
                          const FishKeypoints& kps) const;

private:
    OnnxHelper onnx_;
    int inputW_ = DEFAULT_INPUT_W;
    int inputH_ = DEFAULT_INPUT_H;

    // 从top_k输出解码关键点坐标（映射回原图）
    FishKeypoints decodeTopK(
        const float*   scores,   // [1,17,30]
        const int64_t* indices,  // [1,17,30]
        int heatW, int heatH,    // 热力图尺寸
        const cv::Rect& roi,     // 裁剪区域（原图坐标）
        int cropW, int cropH);   // 送入模型的裁剪图尺寸

    // 颜色表（p1~p15对应不同颜色）
    static cv::Scalar keypointColor(int idx);
};
