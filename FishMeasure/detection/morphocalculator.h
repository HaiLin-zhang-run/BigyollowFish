#pragma once
#include "../core/fishrecord.h"
#include <opencv2/core.hpp>

/**
 * @brief 形态指标计算器
 * 基于关键点像素坐标 + 深度图 → 3D空间真实尺寸(mm)
 *
 * 转换公式 (Astra2 相机内参):
 *   X = (u - cx) * Z / fx
 *   Y = (v - cy) * Z / fy
 *   Z = 深度图像素值 (mm)
 */
class MorphoCalculator {
public:
    struct Intrinsics {
        float fx = 0, fy = 0;  // 焦距（像素）
        float cx = 0, cy = 0;  // 主点（像素）
    };

    /**
     * @brief 设置相机内参（从OrbbecSDK读取后调用一次）
     */
    void setIntrinsics(const Intrinsics& K) { K_ = K; }

    /**
     * @brief 计算全部形态指标
     * @param kps      17关键点（含像素坐标）
     * @param depthMap 对齐到RGB的深度图 CV_16U (mm)
     * @return FishMorphology（单位mm，体重留空由串口秤填）
     */
    FishMorphology calculate(FishKeypoints& kps,
                             const cv::Mat& depthMap,
                             int colorW, int colorH);

    /**
     * @brief 从深度图采样关键点深度
     * 使用3×3邻域中值滤波剔除噪声
     */
    void sampleDepths(FishKeypoints& kps,
                      const cv::Mat& depthMap,
                      int colorW, int colorH) const;

private:
    Intrinsics K_;

    // 像素坐标 + 深度 → 三维点(mm)
    cv::Point3f to3D(const cv::Point2f& px, float Z) const;

    // 两关键点3D欧氏距离(mm)
    float dist3D(int a, int b,
                 const FishKeypoints& kps) const;

    // 某点处的垂直距离（背-腹，需要背腹两侧点）
    float vertDist(const cv::Point3f& top,
                   const cv::Point3f& bot) const;

    // 从深度图取中值深度
    float sampleDepth(const cv::Mat& depthMap,
                      const cv::Point2f& pt, 
                      int colorW, int colorH, 
                      int radius=3) const;
};
