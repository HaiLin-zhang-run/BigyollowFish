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

    // ── 基于关键点的3D距离与垂直高度 ──
    m.totalLength        = dist3D(1, 7,  kps); // 1: 全长 p1->p7
    m.bodyLength         = dist3D(1, 8,  kps); // 2: 体长 p1->p8
    m.headLength         = dist3D(1, 15, kps); // 3: 头长 p1->p15
    m.trunkLength        = dist3D(15,12, kps); // 4: 躯干长 p15->p12
    m.tailLength         = dist3D(12, 8, kps); // 5: 尾长 p12->p8
    m.snoutLength        = dist3D(1, 2,  kps); // 6: 吻长 p1->p2
    m.eyeLength          = dist3D(2, 3,  kps); // 7: 眼长 p2->p3
    m.postEyeHeadLength  = dist3D(3, 15, kps); // 8: 眼后头长 p3->p15
    m.caudPedLength      = dist3D(10, 8, kps); // 9: 尾柄长 p10->p8
    m.pectoralFinLength  = dist3D(4, 14, kps); // 12: 胸鳍长 p4->p14
    m.caudalFinLength    = dist3D(8, 7,  kps); // 13: 尾鳍长 p8->p7
    m.analFinLength      = dist3D(12,10, kps); // 14: 臀鳍长 p12->p10

    // ── 垂直高度（背腹方向，利用Y轴坐标差）──
    // 10: 体高：p5到 p13
    if (kps.conf[4] > 0.1f && kps.conf[12] > 0.1f) {
        auto p5_3d  = to3D(kps.p(5),  kps.d(5));
        auto p13_3d = to3D(kps.p(13), kps.d(13));
        m.bodyHeight = vertDist(p5_3d, p13_3d);
    }
    
    // 11: 尾柄高：p6 到 p9
    if (kps.conf[5] > 0.1f && kps.conf[8] > 0.1f) {
        auto p6_3d = to3D(kps.p(6), kps.d(6));
        auto p9_3d = to3D(kps.p(9), kps.d(9));
        m.caudPedHeight = vertDist(p6_3d, p9_3d); 
    }

    // ── 厚度：寻找深度图中最靠近相机的点与鱼体背景(外缘)深度的差 ──
    float z_top = 10000.0f;
    float z_bg = 0.0f;
    int minX = colorW, minY = colorH, maxX = 0, maxY = 0;
    
    // 1. 确定鱼体在彩色图上的 Bounding Box
    for (int i = 0; i < 17; ++i) {
        if (kps.conf[i] > 0.2f && kps.pts[i].x > 0 && kps.pts[i].y > 0) {
            minX = std::min(minX, (int)kps.pts[i].x);
            minY = std::min(minY, (int)kps.pts[i].y);
            maxX = std::max(maxX, (int)kps.pts[i].x);
            maxY = std::max(maxY, (int)kps.pts[i].y);
        }
    }
    
    // 2. 将 BB 转换到深度图坐标系，并在该区域内寻找最大与最小有效深度
    if (maxX > minX && maxY > minY && !depthMap.empty()) {
        float scaleX = (float)colorW / depthMap.cols;
        float scaleY = (float)colorH / depthMap.rows;
        
        // 适当向内收缩边界，避免采集到过多背景板
        int u0 = std::max(0, (int)std::round((minX + 10) / scaleX));
        int v0 = std::max(0, (int)std::round((minY + 10) / scaleY));
        int u1 = std::min(depthMap.cols - 1, (int)std::round((maxX - 10) / scaleX));
        int v1 = std::min(depthMap.rows - 1, (int)std::round((maxY - 10) / scaleY));

        for (int v = v0; v <= v1; ++v) {
            for (int u = u0; u <= u1; ++u) {
                float d = depthMap.at<uint16_t>(v, u);
                if (d > 100 && d < 3000) { // 限制合理探测范围 (10cm ~ 3m)
                    z_top = std::min(z_top, d);
                    z_bg  = std::max(z_bg, d);
                }
            }
        }
    }
    
    // 3. 计算厚度差值
    if (z_top < 3000.0f && z_bg > z_top) {
        m.thickness = z_bg - z_top;
    } else {
        m.thickness = 0;
    }

    // 体重：由外部串口秤提供
    m.weight = 0.0f;

    return m;
}
