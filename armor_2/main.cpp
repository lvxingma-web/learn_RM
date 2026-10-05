#include "inference.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

//PnP结算：图中四点、真实四点、相机参数
double calculateDistance(const Armor& armor, const cv::Mat& cameraMatrix,
                         const cv::Mat& distortion)
{
    double width = armor.large ? 230.0 : 135.0;
    double height = 55.0;

    std::vector<cv::Point3d> realPoints = {
        {-width / 2, -height / 2, 0}, {width / 2, -height / 2, 0},
        {width / 2, height / 2, 0}, {-width / 2, height / 2, 0}
    };
    std::vector<cv::Point2f> imagePoints(armor.points.begin(), armor.points.end());
    std::vector<cv::Mat> rotations;
    std::vector<cv::Mat> translations;
    cv::solvePnPGeneric(realPoints, imagePoints, cameraMatrix, distortion,
                        rotations, translations, false, cv::SOLVEPNP_IPPE);

    //平面四点可能得到两个候选解，选在相机前方且重投影误差较小的那个
    double bestError = std::numeric_limits<double>::infinity();
    double distance = -1;
    for (size_t solution = 0; solution < translations.size(); ++solution)
    {
        const cv::Mat& translation = translations[solution];
        const cv::Mat& rotationVector = rotations[solution];
        if (!cv::checkRange(translation) || !cv::checkRange(rotationVector))
        {
            continue;
        }
        cv::Mat rotationMatrix;
        cv::Rodrigues(rotationVector, rotationMatrix);
        bool inFront = true;
        for (const cv::Point3d& point : realPoints)
        {
            double depth = rotationMatrix.at<double>(2, 0) * point.x
                           + rotationMatrix.at<double>(2, 1) * point.y
                           + translation.at<double>(2, 0);
            if (depth <= 0)
            {
                inFront = false;
            }
        }
        if (!inFront)
        {
            continue;
        }
        //让这个候选解把真实点投影回图片，看看与检测点相差多少
        std::vector<cv::Point2d> projectedPoints;
        cv::projectPoints(realPoints, rotationVector, translation, cameraMatrix,
                          distortion, projectedPoints);
        double error = 0;
        for (int point = 0; point < 4; ++point)
        {
            cv::Point2d difference = projectedPoints[point] - cv::Point2d(imagePoints[point]);
            error += difference.dot(difference);
        }
        if (error < bestError)
        {
            bestError = error;
            distance = cv::norm(translation) / 1000.0;
        }
    }
    return distance;
}

void drawArmor(cv::Mat& displayFrame, const Armor& armor)
{
    cv::Scalar color = cv::Scalar(180, 180, 180); //熄灭/未识别时灰色
    if (armor.color == "blue")
    {
        color = cv::Scalar(255, 160, 0);
    }
    else if (armor.color == "red")
    {
        color = cv::Scalar(0, 0, 255);
    }
    else if (armor.color == "purple")
    {
        color = cv::Scalar(255, 0, 255);
    }
    for (int point = 0; point < 4; ++point)
    {
        cv::line(displayFrame, armor.points[point], armor.points[(point + 1) % 4], color, 2);
        cv::circle(displayFrame, armor.points[point], 5, cv::Scalar(0, 255, 255), -1);
        cv::putText(displayFrame, "P" + std::to_string(point),
                    armor.points[point] + cv::Point2f(6, -6), cv::FONT_HERSHEY_SIMPLEX,
                    0.6, cv::Scalar(0, 255, 255), 2);
    }

    //标注装甲板信息
    std::ostringstream title;
    title << armor.color << " " << armor.category << " "
          << (armor.large ? "large" : "small") << " ";
    if (armor.distanceMeters >= 0)
    {
        title << std::fixed << std::setprecision(2) << armor.distanceMeters << "m";
    }
    else
    {
        title << "PnP failed";
    }
    cv::Point2f textPosition = armor.points[0];
    textPosition.x = std::clamp(textPosition.x, 0.0f, float(displayFrame.cols - 1));
    textPosition.y = std::clamp(textPosition.y - 70, 25.0f, float(displayFrame.rows - 30));
    cv::putText(displayFrame, title.str(), textPosition,
                cv::FONT_HERSHEY_SIMPLEX, 0.7, color, 2);

    std::ostringstream classifierText;
    classifierText << "classifier index=" << armor.classifierIndex << " score="
                   << std::fixed << std::setprecision(2) << armor.classifierScore;
    cv::putText(displayFrame, classifierText.str(), textPosition + cv::Point2f(0, 26),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
}

int main()
{
    try
    {
        std::string projectPath = PROJECT_DIRECTORY;//项目路径
        std::string videoPath = projectPath + "/demo.avi";
        //窗口大小
        const int windowWidth = 960;
        const int windowHeight = 720;

        //加载YOLO和分类器模型
        cv::dnn::Net yolo = cv::dnn::readNetFromONNX(projectPath + "/models/yolo_complete.onnx");
        yolo.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        yolo.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        yolo.enableFusion(false);
        cv::dnn::Net classifier = cv::dnn::readNetFromONNX(projectPath + "/models/classifier.onnx");

        //读取相机参数
        cv::FileStorage calibration(projectPath + "/camera.yml", cv::FileStorage::READ);
        if (!calibration.isOpened())
        {
            throw std::runtime_error("Cannot open camera.yml");
        }
        cv::Mat cameraMatrix;
        cv::Mat distortion;
        calibration["camera_matrix"] >> cameraMatrix;
        calibration["distortion"] >> distortion;
        if (cameraMatrix.rows != 3 || cameraMatrix.cols != 3 || cameraMatrix.type() != CV_64F ||
            distortion.total() != 5 || distortion.type() != CV_64F ||
            !cv::checkRange(cameraMatrix) || !cv::checkRange(distortion))
        {
            throw std::runtime_error("Invalid camera calibration");
        }

        cv::VideoCapture video(videoPath);
        if (!video.isOpened())
        {
            throw std::runtime_error("Cannot open video: " + videoPath);
        }
        cv::namedWindow("armor decoded", cv::WINDOW_NORMAL);
        cv::resizeWindow("armor decoded", windowWidth, windowHeight);

        cv::Mat frame;
        int frameCount = 0;//读取到的帧数
        int armorCount = 0;//总共读取到装甲板的数量
        int distanceCount = 0;//读取到有效距离的总数
        while (video.read(frame))
        {
            //用YOLO模型读出当前帧中装甲版的四点，并确定装甲板的大小
            std::vector<Armor> armors = detectArmors(yolo, frame);
            for (Armor& armor : armors)
            {
                //用分类器判断装甲板的类别
                classifyArmor(classifier, frame, armor);
                //PnP解算装甲板的距离
                armor.distanceMeters = calculateDistance(armor, cameraMatrix, distortion);
                if (armor.distanceMeters >= 0)
                {
                    ++distanceCount;
                }
            }

            cv::Mat displayFrame = frame.clone();
            for (const Armor& armor : armors)
            {
                drawArmor(displayFrame, armor);
            }
            ++frameCount;
            armorCount += int(armors.size());
            cv::putText(displayFrame, "frame=" + std::to_string(frameCount), {20, 35},
                        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);

            double scale = std::min(1.0, std::min(double(windowWidth) / frame.cols,
                                                 double(windowHeight) / frame.rows));
            cv::Mat preview;
            cv::resize(displayFrame, preview,
                       cv::Size(cvRound(frame.cols * scale), cvRound(frame.rows * scale)),
                       0, 0, cv::INTER_AREA);
            cv::imshow("armor decoded", preview);
            int key = cv::waitKey(1) & 0xff;
            if (key == 27 || key == 'q')
            {
                break;
            }
            if (frameCount % 30 == 0)
            {
                std::cout << "processed " << frameCount << " frames\n";
            }
        }
        cv::destroyAllWindows();
        std::cout << "frames=" << frameCount << " armors=" << armorCount
                  << " distances=" << distanceCount << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
