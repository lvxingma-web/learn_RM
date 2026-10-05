#include "inference.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

static const std::string colors[4] = {"blue", "red", "extinguish", "purple"};
static const std::string categories[9] = {
    "sentry", "one", "two", "three", "four", "five", "outpost", "base", "not_armor"
};

std::vector<Armor> detectArmors(cv::dnn::Net& yolo, const cv::Mat& frame)
{
    //按比例缩小，贴到640x640的画布上
    double scale = std::min(640.0 / frame.cols, 640.0 / frame.rows);
    int width = std::clamp(cvRound(frame.cols * scale), 1, 640);
    int height = std::clamp(cvRound(frame.rows * scale), 1, 640);
    cv::Mat input = cv::Mat::zeros(640, 640, CV_8UC3);
    cv::resize(frame, input(cv::Rect(0, 0, width, height)), cv::Size(width, height));
    yolo.setInput(cv::dnn::blobFromImage(input, 1.0 / 255, cv::Size(640, 640),
                                       cv::Scalar(), true, false));

    //输出[1,25200,22]，每一行就是一块候选的22个数。
    cv::Mat output = yolo.forward();
    if (output.type() != CV_32F || output.dims != 3 || output.size[0] != 1 ||
        output.size[1] != 25200 || output.size[2] != 22)
    {
        throw std::runtime_error("Complete YOLO must return [1,25200,22]");
    }
    //将三维数组转换成二维数组
    cv::Mat rows = output.reshape(1, output.size[1]);
    const float scoreThreshold = 0.35f;
    std::vector<Armor> candidates;
    std::vector<cv::Rect> boxes;
    std::vector<float> scores;

    //逐行读取25200行的22个数，比较出颜色、类别的最高分数
    for (int row = 0; row < rows.rows; ++row)
    {
        const float* values = rows.ptr<float>(row);
        //sigmoid将目标分数转化为0~1的数
        float score = 1.0f / (1.0f + std::exp(-std::clamp(values[8], -30.0f, 30.0f)));
        if (score < scoreThreshold)
        {
            continue;
        }
        int colorField = 9;
        int categoryField = 13;
        for (int field = 10; field < 13; ++field)
        {
            if (values[field] > values[colorField])
            {
                colorField = field;
            }
        }
        for (int field = 14; field < 22; ++field)
        {
            if (values[field] > values[categoryField])
            {
                categoryField = field;
            }
        }
        if (categoryField - 13 == 8)
        {
            continue;
        }

        //前8个数直接组成四点 再除缩放比例回到原图
        cv::Point2f modelPoints[4];
        bool valid = true;
        for (int point = 0; point < 4; ++point)
        {
            modelPoints[point] = cv::Point2f(values[point * 2], values[point * 2 + 1]) / scale;
            if (!std::isfinite(modelPoints[point].x) || !std::isfinite(modelPoints[point].y))
            {
                valid = false;
            }
        }
        if (!valid)
        {
            continue;
        }
        Armor armor;
        //模型顺序左上、左下、右下、右上 -> 程序顺序左上、右上、右下、左下
        armor.points = {modelPoints[0], modelPoints[3], modelPoints[2], modelPoints[1]};
        armor.color = colors[colorField - 9];
        armor.category = categories[categoryField - 13];
        armor.yoloScore = score;
        std::vector<cv::Point2f> polygon(armor.points.begin(), armor.points.end());
        if (!cv::isContourConvex(polygon) || cv::contourArea(polygon) < 20)
        {
            continue;
        }
        cv::Rect box = cv::boundingRect(polygon);
        if ((box & cv::Rect(0, 0, frame.cols, frame.rows)).area() < box.area() * 0.7)
        {
            continue;
        }

        //判断大小装甲板
        cv::Point2f leftCenter = (armor.points[0] + armor.points[3]) * 0.5f;
        cv::Point2f rightCenter = (armor.points[1] + armor.points[2]) * 0.5f;
        //用真实大小装甲板的平均值作为判断条件
        double lightHeight = (cv::norm(armor.points[3] - armor.points[0])
                              + cv::norm(armor.points[2] - armor.points[1])) / 2;
        double boundary = (135.0 / 55.0 + 230.0 / 55.0) / 2;
        armor.large = lightHeight > 0 && cv::norm(rightCenter - leftCenter) / lightHeight > boundary;
        candidates.push_back(armor);
        boxes.push_back(box);
        scores.push_back(score);
    }

    //NMS去除重复候选
    std::vector<int> keptIndices;
    cv::dnn::NMSBoxes(boxes, scores, scoreThreshold, 0.4f, keptIndices);
    std::vector<Armor> results;
    for (int index : keptIndices)
    {
        results.push_back(candidates[index]);
    }
    return results;
}

void classifyArmor(cv::dnn::Net& classifier, const cv::Mat& frame, Armor& armor)
{
    //把装甲拉正到128×64，然后裁中间图案
    const cv::Point2f destination[4] = {{0, 0}, {127, 0}, {127, 63}, {0, 63}};
    cv::Mat rectified;
    cv::Mat transform = cv::getPerspectiveTransform(armor.points.data(), destination);
    cv::warpPerspective(frame, rectified, transform, cv::Size(128, 64));
    cv::Mat gray;
    cv::cvtColor(rectified(cv::Rect(30, 4, 68, 56)), gray, cv::COLOR_BGR2GRAY);

    //按比例缩小，贴到32x32的图上
    double scale = std::min(32.0 / gray.cols, 32.0 / gray.rows);
    int width = std::clamp(cvRound(gray.cols * scale), 1, 32);
    int height = std::clamp(cvRound(gray.rows * scale), 1, 32);
    cv::Mat patch = cv::Mat::zeros(32, 32, CV_8U);
    cv::resize(gray, patch(cv::Rect(0, 0, width, height)), cv::Size(width, height));

    //执行分类器，输出九个数
    classifier.setInput(cv::dnn::blobFromImage(patch, 1.0 / 255));
    cv::Mat output = classifier.forward();
    if (output.type() != CV_32F || output.total() != 9)
    {
        throw std::runtime_error("Classifier must return nine float scores");
    }
    cv::Mat scores = output.reshape(1, 1);
    double minimum;
    double maximum;
    cv::minMaxLoc(scores, &minimum, &maximum);
    cv::Mat probabilities;
    if (minimum < 0 || maximum > 1 || std::abs(cv::sum(scores)[0] - 1) > 0.05)
    {
        //把九个数转化为总和为1的数
        cv::exp(scores - maximum, probabilities);
        probabilities /= cv::sum(probabilities)[0];
    }
    else
    {
        probabilities = scores;
    }

    //九选一
    int bestIndex = 0;
    for (int index = 1; index < 9; ++index)
    {
        if (probabilities.at<float>(0, index) > probabilities.at<float>(0, bestIndex))
        {
            bestIndex = index;
        }
    }
    armor.classifierIndex = bestIndex;
    armor.classifierScore = probabilities.at<float>(0, bestIndex);
}
