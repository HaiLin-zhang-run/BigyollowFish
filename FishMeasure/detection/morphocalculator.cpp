#include "morphocalculator.h"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <algorithm>

// ─── 内参转换 ─────────────────────────────────
cv::Point3f MorphoCalculator::to3D(
    const cv::Point2f& px, float Z) const
{
    if (Z <= 0) return {0,0,0};
    return {
        (px.x - K_.cx) * Z / K_.fx,
        (px.y - K_.cy) * Z / K_.fy,
        Z
    };
}

float MorphoCalculator::dist3D(
    int a, int b, const FishKeypoints& kps) const
{
    // a,b 为 1-indexed 关键点序号
    auto p3a = to3D(kps.p(a), kps.d(a));
    auto p3b = to3D(kps.p(b), kps.d(b));
    return (float)cv::norm(p3a - p3b);
}

float MorphoCalculator::vertDist(
    const cv::Point3f& top,
    const cv::Point3f& bot) const
{
    return std::abs(top.y - bot.y); // Y轴方向为高度
}

float MorphoCalculator::sampleDepth(
    const cv::Mat& depthMap,
    const cv::Point2f& pt, int colorW, int colorH, int radius) const
{
    if (depthMap.empty() || colorW <= 0 || colorH <= 0) return 0;
    
    float scaleX = (float)colorW / depthMap.cols;
    float scaleY = (float)colorH / depthMap.rows;
    
    int u = (int)std::round(pt.x / scaleX);
    int v = (int)std::round(pt.y / scaleY);
    
    int x0 = std::max(0, u-radius), x1 = std::min(depthMap.cols-1, u+radius);
    int y0 = std::max(0, v-radius), y1 = std::min(depthMap.rows-1, v+radius);

    std::vector<float> vals;
    vals.reserve((2*radius+1)*(2*radius+1));
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            float d = depthMap.at<uint16_t>(y, x);
            if (d > 50 && d < 5000) vals.push_back(d); // 有效深度范围
        }

    if (vals.empty()) return 0;
    std::sort(vals.begin(), vals.end());
    return vals[vals.size()/2]; // 中值
}

void MorphoCalculator::sampleDepths(
    FishKeypoints& kps, const cv::Mat& depthMap, int colorW, int colorH) const
{
    for (int i = 0; i < 17; ++i)
        kps.depthMM[i] = sampleDepth(depthMap, kps.pts[i], colorW, colorH);
}

// ─── 主计算函数 ────────────────────────────────
FishMorphology MorphoCalculator::calculate(
    FishKeypoints& kps, const cv::Mat& depthMap, int colorW, int colorH)
{
    // 1. 采样各关键点深度
    sampleDepths(kps, depthMap, colorW, colorH);

    FishMorphology m;

    // ── 基于关键点的3D距离 ──
    // 关键点对应规范(p1=吻端, p7=尾末, p8=尾叉, p9=尾柄始 ...)
    m.totalLength    = dist3D(1, 7,  kps); // 全长
    m.forkLength     = dist3D(1, 8,  kps); // 叉长
    m.bodyLength     = dist3D(1, 9,  kps); // 体长
    m.snoutLength    = dist3D(1, 3,  kps); // 吻长
    m.eyeDiameter    = dist3D(2, 3,  kps); // 眼径
    m.headLength     = dist3D(1, 4,  kps); // 头长
    m.caudPedLength  = dist3D(9, 10, kps); // 尾柄长
    m.dorsalFinLen   = dist3D(5, 6,  kps); // 背鳍长（近似）
    m.analFinLen     = dist3D(10,11, kps); // 臀鳍长

    // ── 垂直高度（背腹方向，利用Y轴坐标差）──
    // 头高：p4处，需要背侧和腹侧两点（简化用p4.y与p13.y差）
    if (kps.conf[3] > 0.1f && kps.conf[12] > 0.1f) {
        auto p4_3d  = to3D(kps.p(4),  kps.d(4));
        auto p13_3d = to3D(kps.p(13), kps.d(13));
        m.headHeight = vertDist(p4_3d, p13_3d);
    }
    // 体高：p5处（背鳍最高点到腹面p13附近）
    if (kps.conf[4] > 0.1f && kps.conf[12] > 0.1f) {
        auto p5_3d  = to3D(kps.p(5),  kps.d(5));
        auto p13_3d = to3D(kps.p(13), kps.d(13));
        m.bodyHeight = vertDist(p5_3d, p13_3d);
    }
    // 尾柄高：p11处
    if (kps.conf[10] > 0.1f && kps.conf[9] > 0.1f) {
        m.caudPedHeight = dist3D(10, 11, kps); // 简化：p10-p11垂直
    }

    // ── 厚度：深度图中鱼体区域的Z范围 ──
    // 取p5(背最高)的Z与腹部Z差作为厚度估算
    if (kps.d(5) > 0 && kps.d(13) > 0)
        m.thickness = std::abs(kps.d(5) - kps.d(13));

    // 体重：由串口秤填写（此处设0）
    m.weight = 0.0f;

    return m;
}
