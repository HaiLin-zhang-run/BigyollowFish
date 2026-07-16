#include "keypointdetector.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>

bool KeypointDetector::load(const std::string& modelPath) {
    if (!onnx_.loadModel(modelPath)) return false;

    // 尝试从模型输入形状推断尺寸
    auto shape = onnx_.inputShape();
    if (shape.size() == 4 && shape[2] > 0 && shape[3] > 0) {
        inputH_ = (int)shape[2];
        inputW_ = (int)shape[3];
    }
    return true;
}

FishKeypoints KeypointDetector::detect(
    const cv::Mat& bgrImg, const cv::Rect& roi)
{
    if (!isLoaded() || bgrImg.empty()) return {};

    // 扩展ROI防止裁剪过紧（padding 10%）
    int pad = (int)(std::max(roi.width, roi.height) * 0.1f);
    cv::Rect paddedRoi = roi;
    paddedRoi.x      = std::max(0, roi.x - pad);
    paddedRoi.y      = std::max(0, roi.y - pad);
    paddedRoi.width  = std::min(bgrImg.cols - paddedRoi.x, roi.width  + 2*pad);
    paddedRoi.height = std::min(bgrImg.rows - paddedRoi.y, roi.height + 2*pad);

    cv::Mat crop = bgrImg(paddedRoi).clone();
    int cropW = crop.cols, cropH = crop.rows;

    // 预处理: 缩放→RGB→/255.0→normalize→CHW
    // PaddlePaddle HRNet通常使用ImageNet均值方差归一化
    auto blob = OnnxHelper::imageToBlob(crop, inputW_, inputH_);

    // 额外: ImageNet 归一化
    float mean[3] = {0.485f, 0.456f, 0.406f};
    float std_[3] = {0.229f, 0.224f, 0.225f};
    for (int c = 0; c < 3; ++c)
        for (int i = 0; i < inputW_*inputH_; ++i)
            blob[c*inputW_*inputH_ + i] =
                (blob[c*inputW_*inputH_ + i] - mean[c]) / std_[c];

    // 推理
    std::vector<int64_t> shape = {1, 3, inputH_, inputW_};
    auto outputs = onnx_.run(blob, shape);

    // 模型实际有4个输出:
    // outputs[0]: bilinear_interp 热力图 [1,17,H,W]  H,W == inputH, inputW（1:1，不是1/4!）
    // outputs[1]: unsqueeze2_0.tmp_0     [1,17,H,W,1] (中间结果，不是scores)
    // outputs[2]: top_k_v2_0.tmp_0       [1,17,30]    top_k scores
    // outputs[3]: top_k_v2_0.tmp_1       [1,17,30]    top_k flat indices
    // 使用top_k直接解码（无需手动argmax）

    if ((int)outputs.size() < 4) return {};

    const float*   scores  = outputs[2].GetTensorData<float>();
    const int64_t* indices = outputs[3].GetTensorData<int64_t>();

    // 热力图尺寸（从output[0]的shape获取）
    // 注意：此模型热力图与输入等尺寸（1:1），不是HRNet常见的1/4下采样！
    auto heatShape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
    int heatH = (heatShape.size() >= 3) ? (int)heatShape[2] : inputH_;
    int heatW = (heatShape.size() >= 4) ? (int)heatShape[3] : inputW_;

    return decodeTopK(scores, indices, heatW, heatH,
                      paddedRoi, cropW, cropH);
}

FishKeypoints KeypointDetector::decodeTopK(
    const float*   scores,
    const int64_t* indices,
    int heatW, int heatH,
    const cv::Rect& roi,
    int cropW, int cropH)
{
    FishKeypoints kps;
    int K = 30; // top-k数量

    for (int kp = 0; kp < 17; ++kp) {
        // 取该关键点的第0个（最高置信度）候选
        float  bestScore = scores [kp * K + 0];
        int64_t flatIdx  = indices[kp * K + 0];

        // flat_index → 热力图(row, col)
        int heatRow = (int)(flatIdx / heatW);
        int heatCol = (int)(flatIdx % heatW);

        // 热力图坐标 → input 坐标 (由于直接拉伸缩放，热力图等比例对应 crop 区域)
        int inputW = heatW; 
        int inputH = heatH;
        
        float inputX = (heatCol + 0.5f);
        float inputY = (heatRow + 0.5f);
        
        // 直接缩放比例
        float scaleX = (float)cropW / inputW;
        float scaleY = (float)cropH / inputH;

        // input坐标 → crop坐标
        float cropX = inputX * scaleX;
        float cropY = inputY * scaleY;

        // crop坐标 → 原图坐标
        float origX = roi.x + cropX;
        float origY = roi.y + cropY;

        kps.pts[kp]     = {origX, origY};
        kps.conf[kp]    = bestScore;
        kps.depthMM[kp] = 0.0f; // 后续由MorphoCalculator填充
    }
    return kps;
}

void KeypointDetector::drawKeypoints(
    cv::Mat& img, const FishKeypoints& kps) const
{
    for (int i = 0; i < 15; ++i) {  // p1~p15
        if (kps.conf[i] < 0.1f) continue;
        cv::Scalar color = keypointColor(i);
        cv::circle(img, kps.pts[i], 5, color, -1);
        cv::circle(img, kps.pts[i], 6, {255,255,255}, 1);
        // 标注点号
        char label[8]; snprintf(label, 8, "p%d", i+1);
        cv::putText(img, label,
            cv::Point((int)kps.pts[i].x + 6, (int)kps.pts[i].y - 6),
            cv::FONT_HERSHEY_SIMPLEX, 0.4, {255,255,255}, 1);
    }
}

void KeypointDetector::drawMeasureLines(
    cv::Mat& img, const FishKeypoints& kps) const
{
    // 绘制主测量线（参考视频图2中的蓝色连线）
    auto drawLine = [&](int a, int b, cv::Scalar color) {
        if (kps.conf[a-1] > 0.1f && kps.conf[b-1] > 0.1f)
            cv::line(img, kps.pts[a-1], kps.pts[b-1], color, 2);
    };

    drawLine(1, 7, {255, 50, 50});  // 全长（蓝色）
    drawLine(1, 4, {50, 200, 50});  // 头长（绿色）
    drawLine(1, 3, {50, 50, 255}); // 吻长（红色）
}

cv::Scalar KeypointDetector::keypointColor(int idx) {
    // 彩色关键点颜色表
    static const cv::Scalar colors[] = {
        {255, 50, 50},   // p1 吻端 红
        {255, 130, 50},  // p2 眼前缘
        {255, 200, 50},  // p3 眼后缘
        {200, 255, 50},  // p4 鳃盖后
        {50, 255, 50},   // p5 背鳍高
        {50, 255, 200},  // p6
        {50, 200, 255},  // p7 尾末端
        {50, 130, 255},  // p8 尾叉
        {50, 50, 255},   // p9 尾柄
        {130, 50, 255},  // p10
        {200, 50, 255},  // p11
        {255, 50, 200},  // p12
        {255, 50, 130},  // p13
        {255, 100, 100}, // p14
        {200, 200, 50},  // p15
    };
    if (idx < 0 || idx >= 15) return {200, 200, 200};
    return colors[idx];
}
