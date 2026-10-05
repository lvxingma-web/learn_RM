#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <array>
#include <string>
#include <vector>

struct Armor
{
    std::array<cv::Point2f, 4> points; //P0左上、P1右上、P2右下、P3左下
    std::string color;                //YOLO 预测的颜色
    std::string category;             //YOLO 预测的类别名字
    float yoloScore = 0;              //目标分数
    bool large = false;               //由两灯间距/灯高初判大小
    int classifierIndex = -1;         //独立分类器的类别下标
    float classifierScore = 0;        //独立分类器最大概率
    double distanceMeters = -1;       //-1 表示 PnP 未得到有效解
};

std::vector<Armor> detectArmors(cv::dnn::Net& yolo, const cv::Mat& frame);
void classifyArmor(cv::dnn::Net& classifier, const cv::Mat& frame, Armor& armor);
