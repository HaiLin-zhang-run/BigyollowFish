#pragma once
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>
#include <opencv2/core.hpp>

/**
 * @brief ONNX Runtime 推理封装
 * 提供统一的模型加载和前向推理接口
 */
class OnnxHelper {
public:
    OnnxHelper();
    ~OnnxHelper();

    /**
     * @brief 加载ONNX模型
     * @param modelPath 模型文件路径
     * @return true=成功
     */
    bool loadModel(const std::string& modelPath);

    bool isLoaded() const { return session_ != nullptr; }

    // 输入输出节点名
    const char* inputName()  const { return inputNames_[0]; }
    const char* outputName(int i=0) const { return outputNames_[i]; }
    int numOutputs() const { return (int)outputNames_.size(); }

    // 获取输入形状
    std::vector<int64_t> inputShape() const;

    /**
     * @brief 运行推理
     * @param inputData 输入数据(已预处理的float数组)
     * @param inputShape 输入形状 [N,C,H,W]
     * @return 多个输出张量的数据
     */
    std::vector<Ort::Value> run(
        const std::vector<float>& inputData,
        const std::vector<int64_t>& inputShape);

    /**
     * @brief 便捷接口：图像预处理为模型输入
     * 缩放到targetSize, BGR→RGB, /255.0, NHWC→NCHW
     */
    static std::vector<float> imageToBlob(
        const cv::Mat& bgrImg,
        int targetW, int targetH,
        bool normalize = true,  // /255.0
        bool swapRB = true);    // BGR→RGB

private:
    Ort::Env              env_;
    Ort::SessionOptions   sessionOpts_;
    Ort::Session*         session_  = nullptr;
    Ort::AllocatorWithDefaultOptions allocator_;

    std::vector<const char*> inputNames_;
    std::vector<const char*> outputNames_;
    std::vector<Ort::AllocatedStringPtr> inputNameBufs_;
    std::vector<Ort::AllocatedStringPtr> outputNameBufs_;
};
