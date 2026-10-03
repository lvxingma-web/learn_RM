#pragma once  // 同一个编译单元中只包含本头文件一次。

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// 加试 1：不再用一个估算的装甲板 RotatedRect 存四点。
// 这里每个点都对应一根灯条的末端中心。
struct LightBar {
    cv::RotatedRect rect;   // 灯条轮廓的旋转外接矩形。
    cv::Point2f top;        // 上端中心，图像像素坐标。
    cv::Point2f bottom;     // 下端中心，图像像素坐标。
    std::string color;     // blue 或 red。
    float length;          // 灯条长边的像素长度。
};

struct ArmorPlate {
    std::array<cv::Point2f, 4> points;  // P0左上、P1右上、P2右下、P3左下。
    std::string color = "unknown";     // 颜色文字。
    std::string number = "unknown";    // 教学版的编号文字，缺模型时保持未知。
    std::string size;                  // small 或 large。
    float confidence = 0;             // 检测分数，不是实际准确率。
    bool automaticSize = true;        // 是否使用大小自动初判。
};

// 找两条短边，取短边的中点，就得到灯条两端。
// points() 的第 0 个顶点不保证是左上，不能直接照它的顺序使用。
inline std::pair<cv::Point2f, cv::Point2f> lightEnds(const cv::RotatedRect& r) {
    cv::Point2f p[4];  // 接收旋转矩形的四个顶点。
    r.points(p);
    // 比较相邻两条边，edge 指向较短的那条。
    const int edge = cv::norm(p[1] - p[0]) <= cv::norm(p[2] - p[1]) ? 0 : 1;
    // 一条短边和它对面短边的中点，分别是灯条的两端。
    cv::Point2f a = (p[edge] + p[(edge + 1) % 4]) * 0.5f;
    cv::Point2f b = (p[(edge + 2) % 4] + p[(edge + 3) % 4]) * 0.5f;
    if (a.y > b.y) {
        std::swap(a, b);  // 图像里 y 小的一端在上。
    }
    return {a, b};
}

inline std::vector<LightBar> findLights(const cv::Mat& binary, const std::string& color) {
    std::vector<std::vector<cv::Point>> contours;  // 每个轮廓是一组像素点。
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    std::vector<LightBar> result;  // 保存通过筛选的灯条。
    for (const auto& c : contours) {
        if (cv::contourArea(c) < 20) {
            continue;  // 面积太小，跳过当前轮廓。这里保留原参数。
        }
        const auto r = cv::minAreaRect(c);
        const float len = std::max(r.size.width, r.size.height);
        const float wid = std::min(r.size.width, r.size.height);
        if (wid <= 0 || len < 8 || len / wid < 2 || len / wid > 12) {
            continue;  // 排除退化、太短或宽高比不合适的条。
        }
        const auto ends = lightEnds(r);
        // 按 LightBar 字段顺序保存矩形、上下端、颜色、长度。
        result.push_back({r, ends.first, ends.second, color, len});
    }
    return result;
}

inline void setArmorSize(ArmorPlate& a, const std::string& mode) {
    a.automaticSize = mode == "auto";  // 判断是否由程序自动初判。
    if (!a.automaticSize) {
        a.size = mode;  // --type small/large 时直接采用用户指定类型。
        return;
    }
    const auto lc = (a.points[0] + a.points[3]) * 0.5f;  // 左灯中心。
    const auto rc = (a.points[1] + a.points[2]) * 0.5f;  // 右灯中心。
    // 两灯平均像素高度；距离/高度可以消除正面视图的大部分远近影响。
    const double h = (cv::norm(a.points[3] - a.points[0]) +
                      cv::norm(a.points[2] - a.points[1])) / 2.0;
    // 135/55 与 230/55 的中间值，仅为正面视图下的大小初判。
    // 透视会改变比值；做 PnP 验证时可用 --type small/large 指定真实类型。
    const double boundary = (135.0 / 55.0 + 230.0 / 55.0) / 2.0;
    a.size = h > 0 && cv::norm(rc - lc) / h > boundary ? "large" : "small";
}

inline std::vector<ArmorPlate> detectClassic(const cv::Mat& frame,
                                            cv::Mat& blue,
                                            cv::Mat& red,
                                            const std::string& sizeMode) {
    // 这是早期教学版使用的传统灯条路线，实测版默认用 YOLO。
    std::vector<cv::Mat> ch;  // 拆分后依次为 B、G、R。
    cv::split(frame, ch);
    cv::Mat br;
    cv::Mat rb;
    cv::subtract(ch[0], ch[2], br);  // B-R 突出蓝色。
    cv::subtract(ch[2], ch[0], rb);  // R-B 突出红色。
    cv::normalize(br, br, 0, 255, cv::NORM_MINMAX);
    cv::normalize(rb, rb, 0, 255, cv::NORM_MINMAX);
    cv::threshold(br, blue, 100, 255, cv::THRESH_BINARY);
    cv::threshold(rb, red, 100, 255, cv::THRESH_BINARY);
    auto lights = findLights(blue, "blue");
    auto reds = findLights(red, "red");
    lights.insert(lights.end(), reds.begin(), reds.end());  // 合并两种灯条。
    std::vector<ArmorPlate> result;
    for (size_t i = 0; i < lights.size(); ++i) {
        for (size_t j = i + 1; j < lights.size(); ++j) {
            if (lights[i].color != lights[j].color) {
                continue;  // 只让同色灯条配对。
            }
            const LightBar* left = &lights[i];  // 指针引用原列表中的灯条。
            const LightBar* right = &lights[j];
            if (left->rect.center.x > right->rect.center.x) {
                std::swap(left, right);  // 轮廓返回顺序不保证从左到右。
            }
            const float avg = (left->length + right->length) / 2;
            if (std::max(left->length, right->length) /
                    std::min(left->length, right->length) > 1.6f) {
                continue;  // 两灯长度差太大。
            }
            const float dy = std::abs(left->rect.center.y - right->rect.center.y);
            const float dx = right->rect.center.x - left->rect.center.x;
            if (dy > 0.6f * avg || dx < 1.5f * avg || dx > 8.0f * avg) {
                continue;  // 上下错位太多，或左右间距不合适。
            }
            ArmorPlate a;
            a.points = {left->top, right->top, right->bottom, left->bottom};
            a.color = left->color;
            setArmorSize(a, sizeMode);
            result.push_back(a);
        }
    }
    return result;
}

// YOLO 只给四点时，沿两灯中心线采样颜色。颜色差不明显就标 unknown。
inline std::string pointColor(const cv::Mat& frame, const ArmorPlate& a) {
    cv::Mat mask = cv::Mat::zeros(frame.size(), CV_8U);  // 黑处不参与颜色统计。
    // 两条白线覆盖两灯中心线，作为颜色采样区域。
    cv::line(mask, a.points[0], a.points[3], cv::Scalar(255), 5);
    cv::line(mask, a.points[1], a.points[2], cv::Scalar(255), 5);
    const auto mean = cv::mean(frame, mask);  // B、G、R 通道的平均值。
    return mean[0] - mean[2] > 5 ? "blue"
           : mean[2] - mean[0] > 5 ? "red"
                                    : "unknown";
}

struct Camera {
    cv::Mat K;           // 3×3 相机内参矩阵。
    cv::Mat distortion;  // 镜头畸变系数。
    cv::Size imageSize;  // 标定时的图像尺寸，教学版会核对它。
};

// 教学版读 OpenCV YAML 标定；实测版的普通文本读取在 team_armor.cpp。
inline Camera loadCamera(const std::string& path, cv::Size actualSize) {
    cv::FileStorage f(path, cv::FileStorage::READ);
    if (!f.isOpened()) {
        throw std::runtime_error("Cannot read calibration: " + path);
    }
    Camera c;
    f["camera_matrix"] >> c.K;
    f["distortion_coefficients"] >> c.distortion;
    f["image_width"] >> c.imageSize.width;
    f["image_height"] >> c.imageSize.height;
    if (c.K.rows != 3 || c.K.cols != 3 || c.distortion.empty()) {
        throw std::runtime_error("Calibration needs camera_matrix and distortion_coefficients");
    }
    // 统一成 double，后续用 at<double> 读取。
    c.K.convertTo(c.K, CV_64F);
    c.distortion.convertTo(c.distortion, CV_64F);
    if (!cv::checkRange(c.K) || !cv::checkRange(c.distortion) ||
        c.K.at<double>(0, 0) <= 0 || c.K.at<double>(1, 1) <= 0 ||
        std::abs(c.K.at<double>(2, 2) - 1) > 1e-6) {
        throw std::runtime_error("Invalid intrinsics; example file is not a real calibration");
    }
    if (c.imageSize != actualSize) {
        throw std::runtime_error("Calibration resolution differs from video; use matching calibration");
    }
    return c;
}

inline std::vector<cv::Point3f> objectPoints(const std::string& type) {
    const float w = type == "large" ? 230.0f : 135.0f;  // 两灯中心距，mm。
    const float h = 55.0f;                             // 灯条高度，mm。
    // 局部坐标：x 向右、y 向下、z=0，单位 mm，与图像点逐一对应。
    // 原点设在板中心，顺序与 P0～P3 一致。
    return {{-w / 2, -h / 2, 0}, {w / 2, -h / 2, 0},
            {w / 2, h / 2, 0}, {-w / 2, h / 2, 0}};
}

struct Pose {
    bool valid = false;    // 是否找到通过检查的候选解。
    cv::Vec3d rvec{};       // 装甲板局部坐标到相机坐标的旋转向量。
    cv::Vec3d tvec{};       // 板中心在相机坐标中的位置，单位 mm。
    double errorPx = 0;    // 四点重投影 RMS 误差，单位像素。
};

inline Pose estimatePose(const ArmorPlate& a, const Camera& c) {
    // image 是检测到的四个二维像素点，object 是对应的真实毫米点。
    const std::vector<cv::Point2f> image(a.points.begin(), a.points.end());
    const auto object = objectPoints(a.size);
    std::vector<cv::Mat> rotations;
    std::vector<cv::Mat> translations;
    // IPPE 处理平面四点，可能返回多个候选朝向与位置。
    cv::solvePnPGeneric(object, image, c.K, c.distortion, rotations, translations,
                        false, cv::SOLVEPNP_IPPE);
    Pose best;  // 默认为无效；找到合适候选后填写它。
    double minimumError = std::numeric_limits<double>::infinity();
    // 平面 PnP 可能有多个解，保留各角点都在相机前且重投影误差最小的解。
    for (size_t i = 0; i < translations.size(); ++i) {
        const cv::Mat t = translations[i].reshape(1, 3);  // 位置整理成 3 行。
        const cv::Mat r = rotations[i].reshape(1, 3);     // 旋转整理成 3 行。
        cv::Mat rotation;
        cv::Rodrigues(r, rotation);  // 将旋转向量换成 3×3 旋转矩阵。
        bool inFront = true;
        for (const auto& p : object) {
            // 真实点局部 z=0，因此只需 x/y 和平移求其相机深度。
            const double z = rotation.at<double>(2, 0) * p.x +
                             rotation.at<double>(2, 1) * p.y + t.at<double>(2);
            if (z <= 0) {
                inFront = false;
            }
        }
        if (!inFront || !cv::checkRange(t) || !cv::checkRange(r)) {
            continue;  // 排除相机后方或包含无效数值的候选解。
        }

        // 假设此候选姿态为真，把真实四点重新投影回图片。
        std::vector<cv::Point2f> projected;
        cv::projectPoints(object, r, t, c.K, c.distortion, projected);
        double squared = 0;
        for (size_t k = 0; k < image.size(); ++k) {
            const auto d = projected[k] - image[k];  // 每点的二维像素差。
            squared += d.dot(d);                    // dx² + dy²。
        }
        const double error = std::sqrt(squared / image.size());  // RMS。
        if (error < minimumError) {
            // 选取当前误差最小的候选，保存位置与朝向。
            minimumError = error;
            best.valid = true;
            best.errorPx = error;
            for (int k = 0; k < 3; ++k) {
                best.rvec[k] = r.at<double>(k);
                best.tvec[k] = t.at<double>(k);
            }
        }
    }
    return best;
}
