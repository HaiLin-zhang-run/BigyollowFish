#include "fishdetector.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

bool FishDetector::load(const std::string& modelPath) {
    return onnx_.loadModel(modelPath);
}

FishDetector::Detection FishDetector::detect(const cv::Mat& bgrImg) {
    if (!isLoaded() || bgrImg.empty()) return {};

    int srcW = bgrImg.cols, srcH = bgrImg.rows;
    float scale = std::min((float)INPUT_SIZE / srcW, (float)INPUT_SIZE / srcH);
    float padW = (INPUT_SIZE - srcW * scale) / 2.0f;
    float padH = (INPUT_SIZE - srcH * scale) / 2.0f;

    // 预处理: 缩放→RGB→/255.0→CHW
    auto blob = OnnxHelper::imageToBlob(bgrImg, INPUT_SIZE, INPUT_SIZE);

    // 推理
    std::vector<int64_t> shape = {1, 3, INPUT_SIZE, INPUT_SIZE};
    auto outputs = onnx_.run(blob, shape);

    // 获取输出维度信息
    auto outInfo = outputs[0].GetTensorTypeAndShapeInfo();
    auto outShape = outInfo.GetShape();
    const float* data = outputs[0].GetTensorData<float>();

    std::vector<Detection> dets;
    dets.reserve(200);

    if (outShape.size() == 3) {
        int dim1 = (int)outShape[1];
        int dim2 = (int)outShape[2];

        if (dim1 > dim2) {
            // YOLOv5 格式: [1, 25200, features]
            int rows = dim1;
            int cols = dim2;
            for (int i = 0; i < rows; ++i) {
                const float* row = data + i * cols;
                float conf = row[4]; 
                if (conf > 1.0f || conf < 0.0f) conf = 1.0f / (1.0f + std::exp(-conf)); // 兼容 raw logits

                // 如果模型有分类概率 (比如 YOLOv5 标准导出有 obj_conf 和 class_prob)
                if (cols >= 6) {
                    float classProb = row[5];
                    if (classProb > 1.0f || classProb < 0.0f) classProb = 1.0f / (1.0f + std::exp(-classProb));
                    conf *= classProb;
                }

                if (conf < CONF_THRESH) continue;
                
                Detection d;
                float cx = (row[0] - padW) / scale;
                float cy = (row[1] - padH) / scale;
                float w  = row[2] / scale;
                float h  = row[3] / scale;
                d.bbox = cv::Rect2f(cx - w/2, cy - h/2, w, h);
                d.confidence = conf;
                
                int kpFeatures = cols - 5;
                if (NUM_KPS > 0 && kpFeatures >= NUM_KPS * 2) {
                    int stride = kpFeatures / NUM_KPS;
                    for (int k = 0; k < NUM_KPS; ++k) {
                        float kx = (row[5 + k*stride] - padW) / scale;
                        float ky = (row[5 + k*stride + 1] - padH) / scale;
                        d.coarseKps[k] = {kx, ky};
                    }
                }
                dets.push_back(d);
            }
        } else {
            // YOLOv8 格式: [1, features, 8400]
            int features = dim1;
            int anchors = dim2;
            for (int i = 0; i < anchors; ++i) {
                float conf = data[4 * anchors + i]; // YOLOv8 只有 class_prob
                if (conf > 1.0f || conf < 0.0f) conf = 1.0f / (1.0f + std::exp(-conf)); // 兼容 raw logits

                if (conf < CONF_THRESH) continue;

                float cx = (data[0 * anchors + i] - padW) / scale;
                float cy = (data[1 * anchors + i] - padH) / scale;
                float w  = data[2 * anchors + i] / scale;
                float h  = data[3 * anchors + i] / scale;

                Detection d;
                d.bbox = cv::Rect2f(cx - w/2, cy - h/2, w, h);
                d.confidence = conf;

                // 提取关键点: YOLOv8-pose 从 feature 5 开始是 keypoints
                // 每3个值是一个关键点 (x, y, conf) 或者 每2个值 (x, y)
                int kpFeatures = features - 5;
                if (NUM_KPS > 0 && kpFeatures >= NUM_KPS * 2) {
                    int stride = kpFeatures / NUM_KPS; // 2 或 3
                    for (int k = 0; k < NUM_KPS && k * stride + 5 < features; ++k) {
                        float kx = (data[(5 + k * stride) * anchors + i] - padW) / scale;
                        float ky = (data[(5 + k * stride + 1) * anchors + i] - padH) / scale;
                        d.coarseKps[k] = {kx, ky};
                    }
                }
                dets.push_back(d);
            }
        }
    }

    if (dets.empty()) return {};
    return nms(dets);
}

FishDetector::Detection FishDetector::decodeRow(
    const float* row, float scaleX, float scaleY) const
{
    // YOLOv5 decode: sigmoid(cx,cy) * stride + offset, exp(wh) * anchor
    // For export format, coordinates are already in [0, INPUT_SIZE] range
    // after anchor decoding in the model
    float cx = row[0] * scaleX;
    float cy = row[1] * scaleY;
    float w  = row[2] * scaleX;
    float h  = row[3] * scaleY;

    Detection d;
    d.bbox = cv::Rect2f(cx - w/2, cy - h/2, w, h);
    d.confidence = 1.0f / (1.0f + std::exp(-row[4]));

    // 解码5个粗关键点 (row[5..14])
    for (int k = 0; k < NUM_KPS; ++k) {
        float kx = row[5 + k*2]     * scaleX;
        float ky = row[5 + k*2 + 1] * scaleY;
        d.coarseKps[k] = {kx, ky};
    }
    return d;
}

FishDetector::Detection FishDetector::nms(
    std::vector<Detection>& dets) const
{
    // 按置信度降序
    std::sort(dets.begin(), dets.end(),
        [](const Detection& a, const Detection& b){
            return a.confidence > b.confidence;
        });

    // 简单NMS：取最高置信度，抑制IoU>阈值的框
    std::vector<bool> suppressed(dets.size(), false);
    Detection best = dets[0];

    for (size_t i = 1; i < dets.size(); ++i) {
        if (suppressed[i]) continue;
        cv::Rect2f inter = dets[0].bbox & dets[i].bbox;
        float interArea = inter.area();
        float unionArea = dets[0].bbox.area() + dets[i].bbox.area() - interArea;
        if (unionArea > 0 && interArea / unionArea > NMS_THRESH)
            suppressed[i] = true;
    }

    return best; // 返回最高置信度的鱼体框
}

void FishDetector::drawDetection(cv::Mat& img, const Detection& det) const {
    if (!det.isValid()) return;
    cv::Rect r(det.bbox);
    cv::rectangle(img, r, cv::Scalar(0, 200, 0), 2);
    char txt[32];
    snprintf(txt, sizeof(txt), "Fish %.2f", det.confidence);
    cv::putText(img, txt, r.tl() + cv::Point(2, -6),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,200,0}, 2);
}
