#include "onnxhelper.h"
#include <QDebug>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <numeric>

OnnxHelper::OnnxHelper()
    : env_(ORT_LOGGING_LEVEL_WARNING, "FishMeasure")
{
    sessionOpts_.SetIntraOpNumThreads(4);
    sessionOpts_.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_ALL);
}

OnnxHelper::~OnnxHelper() {
    delete session_;
}

bool OnnxHelper::loadModel(const std::string& modelPath) {
    try {
        std::wstring wpath(modelPath.begin(), modelPath.end());
        session_ = new Ort::Session(env_, wpath.c_str(), sessionOpts_);

        // 读取输入节点名
        size_t numIn = session_->GetInputCount();
        for (size_t i = 0; i < numIn; ++i) {
            auto namePtr = session_->GetInputNameAllocated(i, allocator_);
            inputNames_.push_back(namePtr.get());
            inputNameBufs_.push_back(std::move(namePtr));
        }

        // 读取输出节点名
        size_t numOut = session_->GetOutputCount();
        for (size_t i = 0; i < numOut; ++i) {
            auto namePtr = session_->GetOutputNameAllocated(i, allocator_);
            outputNames_.push_back(namePtr.get());
            outputNameBufs_.push_back(std::move(namePtr));
        }
        return true;
    } catch (const Ort::Exception& e) {
        qWarning("ONNX load failed: %s", e.what());
        return false;
    }
}

std::vector<int64_t> OnnxHelper::inputShape() const {
    if (!session_) return {};
    auto info = session_->GetInputTypeInfo(0);
    return info.GetTensorTypeAndShapeInfo().GetShape();
}

std::vector<Ort::Value> OnnxHelper::run(
    const std::vector<float>& inputData,
    const std::vector<int64_t>& inputShape)
{
    auto memInfo = Ort::MemoryInfo::CreateCpu(
        OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memInfo,
        const_cast<float*>(inputData.data()),
        inputData.size(),
        inputShape.data(),
        inputShape.size());

    auto outputs = session_->Run(
        Ort::RunOptions{nullptr},
        inputNames_.data(), &inputTensor, 1,
        outputNames_.data(), outputNames_.size());

    return outputs;
}

std::vector<float> OnnxHelper::imageToBlob(
    const cv::Mat& bgrImg,
    int targetW, int targetH,
    bool normalize, bool swapRB)
{
    // YOLO Letterbox padding
    float scale = std::min((float)targetW / bgrImg.cols, (float)targetH / bgrImg.rows);
    int newW = std::round(bgrImg.cols * scale);
    int newH = std::round(bgrImg.rows * scale);
    
    cv::Mat resized;
    cv::resize(bgrImg, resized, cv::Size(newW, newH));
    
    int top = (targetH - newH) / 2;
    int bottom = targetH - newH - top;
    int left = (targetW - newW) / 2;
    int right = targetW - newW - left;
    
    cv::copyMakeBorder(resized, resized, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));

    if (swapRB) cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);

    resized.convertTo(resized, CV_32F);
    if (normalize) resized /= 255.0f;

    // HWC → CHW (NCHW layout)
    int C = resized.channels(), H = resized.rows, W = resized.cols;
    std::vector<float> blob(C * H * W);

    // Split channels
    std::vector<cv::Mat> chans(C);
    cv::split(resized, chans);
    for (int c = 0; c < C; ++c) {
        std::memcpy(blob.data() + c * H * W,
                    chans[c].data, H * W * sizeof(float));
    }
    return blob;
}
