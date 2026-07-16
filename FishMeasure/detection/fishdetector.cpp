#include "fishdetector.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <QDebug>

bool FishDetector::load(const std::string& modelPath) {
    return onnx_.loadModel(modelPath);
}

FishDetector::Detection FishDetector::detect(const cv::Mat& bgrImg) {
    if (!isLoaded() || bgrImg.empty()) return {};

    int srcW = bgrImg.cols, srcH = bgrImg.rows;
    float scaleX = (float)srcW / INPUT_SIZE;
    float scaleY = (float)srcH / INPUT_SIZE;

    auto blob = OnnxHelper::imageToBlob(bgrImg, INPUT_SIZE, INPUT_SIZE);

    std::vector<int64_t> shape = {1, 3, INPUT_SIZE, INPUT_SIZE};
    auto outputs = onnx_.run(blob, shape);

    auto outInfo = outputs[0].GetTensorTypeAndShapeInfo();
    auto outShape = outInfo.GetShape();
    const float* data = outputs[0].GetTensorData<float>();

    std::vector<Detection> dets;
    dets.reserve(200);

    // [DEBUG LOGGING]
    FILE* logFile = fopen("D:\\QPRO\\debug_log.txt", "a");
    if (logFile) {
        fprintf(logFile, "=== NEW DETECTION ===\n");
        fprintf(logFile, "Shape: ");
        for (auto s : outShape) fprintf(logFile, "%lld ", (long long)s);
        fprintf(logFile, "\n");
        if (outShape.size() >= 3) {
            fprintf(logFile, "First row raw data: ");
            for (int i=0; i<15; i++) fprintf(logFile, "%f ", data[i]);
            fprintf(logFile, "\n");
        }
    }

    if (outShape.size() == 3) {
        int dim1 = (int)outShape[1];
        int dim2 = (int)outShape[2];

        if (dim1 > dim2) {
            int rows = dim1;
            int cols = dim2;
            float maxConfSeen = -9999.0f;
            float maxRawConf = -9999.0f;
            
            for (int i = 0; i < rows; ++i) {
                const float* row = data + i * cols;
                float raw_conf = row[4];
                float conf = raw_conf; 
                if (conf > 1.0f || conf < 0.0f) conf = 1.0f / (1.0f + std::exp(-conf));

                if (conf > maxConfSeen) {
                    maxConfSeen = conf;
                    maxRawConf = raw_conf;
                }
                if (conf < CONF_THRESH) continue;
                
                float cx_raw = row[0], cy_raw = row[1], w_raw = row[2], h_raw = row[3];
                bool isNormalized = (w_raw < 2.0f && h_raw < 2.0f);
                if (isNormalized) {
                    cx_raw *= INPUT_SIZE; cy_raw *= INPUT_SIZE;
                    w_raw *= INPUT_SIZE; h_raw *= INPUT_SIZE;
                }

                Detection d;
                float cx = cx_raw * scaleX;
                float cy = cy_raw * scaleY;
                float w  = w_raw * scaleX;
                float h  = h_raw * scaleY;
                d.bbox = cv::Rect2f(cx - w/2, cy - h/2, w, h);
                d.confidence = conf;
                
                int kpFeatures = cols - 5;
                if (NUM_KPS > 0 && kpFeatures >= NUM_KPS * 2) {
                    int stride = kpFeatures / NUM_KPS;
                    for (int k = 0; k < NUM_KPS; ++k) {
                        float kx_raw = row[5 + k*stride];
                        float ky_raw = row[5 + k*stride + 1];
                        if (isNormalized) {
                            kx_raw *= INPUT_SIZE; ky_raw *= INPUT_SIZE;
                        }
                        d.coarseKps[k] = {kx_raw * scaleX, ky_raw * scaleY};
                    }
                }
                dets.push_back(d);
            }
            if (logFile) fprintf(logFile, "YOLOv5 Max conf: raw=%f, sigmoided=%f, dets=%zu\n", maxRawConf, maxConfSeen, dets.size());
        } else {
            int features = dim1;
            int anchors = dim2;
            float maxConfSeen = -9999.0f;
            for (int i = 0; i < anchors; ++i) {
                float conf = data[4 * anchors + i]; 
                float raw_conf = conf;
                if (conf > 1.0f || conf < 0.0f) conf = 1.0f / (1.0f + std::exp(-conf));

                if (conf > maxConfSeen) maxConfSeen = conf;
                if (conf < CONF_THRESH) continue;

                float cx_raw = data[0 * anchors + i];
                float cy_raw = data[1 * anchors + i];
                float w_raw  = data[2 * anchors + i];
                float h_raw  = data[3 * anchors + i];

                bool isNormalized = (w_raw < 2.0f && h_raw < 2.0f);
                if (isNormalized) {
                    cx_raw *= INPUT_SIZE; cy_raw *= INPUT_SIZE;
                    w_raw *= INPUT_SIZE; h_raw *= INPUT_SIZE;
                }

                Detection d;
                float cx = cx_raw * scaleX;
                float cy = cy_raw * scaleY;
                float w  = w_raw * scaleX;
                float h  = h_raw * scaleY;
                d.bbox = cv::Rect2f(cx - w/2, cy - h/2, w, h);
                d.confidence = conf;

                int kpFeatures = features - 5;
                if (NUM_KPS > 0 && kpFeatures >= NUM_KPS * 2) {
                    int stride = kpFeatures / NUM_KPS;
                    for (int k = 0; k < NUM_KPS && k * stride + 5 < features; ++k) {
                        float kx_raw = data[(5 + k * stride) * anchors + i];
                        float ky_raw = data[(5 + k * stride + 1) * anchors + i];
                        if (isNormalized) {
                            kx_raw *= INPUT_SIZE; ky_raw *= INPUT_SIZE;
                        }
                        d.coarseKps[k] = {kx_raw * scaleX, ky_raw * scaleY};
                    }
                }
                dets.push_back(d);
            }
            if (logFile) fprintf(logFile, "YOLOv8 Max conf: %f, dets=%zu\n", maxConfSeen, dets.size());
        }
    }

    if (logFile) fclose(logFile);

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
