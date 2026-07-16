// Copyright (c) 2026 FishMeasure Project
// SPDX-License-Identifier: MIT
#pragma once

#include <QString>
#include <QDateTime>
#include <QImage>
#include <array>
#include <opencv2/core.hpp>

// ─────────────────────────────────────────────
// 15个鱼体关键点 (对应大黄鱼标注规范 p1~p15)
// ─────────────────────────────────────────────
struct FishKeypoints {
    std::array<cv::Point2f, 17> pts;   // 像素坐标 (原图坐标系)
    std::array<float, 17>       conf;  // 置信度 [0,1]
    std::array<float, 17>       depthMM; // 各点深度 (mm)

    // 便捷访问 p1~p15 (0-indexed: p[0]=p1, p[14]=p15)
    cv::Point2f& p(int i) { return pts[i-1]; }
    const cv::Point2f& p(int i) const { return pts[i-1]; }
    float& d(int i) { return depthMM[i-1]; }
    const float& d(int i) const { return depthMM[i-1]; }

    bool isValid() const { return conf[0] > 0.3f; }
};

// ─────────────────────────────────────────────
// 14项形态测量指标 (mm)
// ─────────────────────────────────────────────
struct FishMorphology {
    float totalLength    = 0; // 全长   p1→p7
    float forkLength     = 0; // 叉长   p1→p8
    float bodyLength     = 0; // 体长   p1→p9
    float snoutLength    = 0; // 吻长   p1→p3
    float eyeDiameter    = 0; // 眼径   p2→p3
    float headLength     = 0; // 头长   p1→p4
    float headHeight     = 0; // 头高   p4处垂直
    float bodyHeight     = 0; // 体高   p5处垂直
    float caudPedLength  = 0; // 尾柄长 p9→p10
    float caudPedHeight  = 0; // 尾柄高 p11处垂直
    float dorsalFinLen   = 0; // 背鳍长 p5→p6
    float pectoralFinLen = 0; // 胸鳍长 p14→附近
    float ventralFinLen  = 0; // 腹鳍长 p13处
    float analFinLen     = 0; // 臀鳍长 p10→p11
    float thickness      = 0; // 厚度   深度图Z差 (mm)
    float weight         = 0; // 体重   串口电子秤 (g)
    float yellowBlueValue = 0; // 黄蓝值 (b*通道)

    bool isValid() const { return totalLength > 10.0f; }
};

// ─────────────────────────────────────────────
// 单条鱼类记录
// ─────────────────────────────────────────────
struct FishRecord {
    QString       id;              // "测试鱼001"
    QDateTime     timestamp;
    QImage        rgbImage;        // 原始RGB帧
    QImage        annotatedImage;  // 带关键点+测量线标注
    FishKeypoints keypoints;
    FishMorphology morphology;
    cv::Mat       depthMat;        // 深度图 (CV_16U, mm)

    // 序列号自动生成
    static QString generateId(int seq) {
        return QString("测试鱼%1").arg(seq, 3, 10, QChar('0'));
    }
};
