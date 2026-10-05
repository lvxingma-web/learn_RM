// 实测版入口：视频 -> YOLO 四点 -> 独立编号分类 -> PnP -> 画面和 CSV。
// 这里加载的是从素材包 yolov5.onnx 提取的三个 float32 检测头。
// 原模型的 FP16/五维后处理在本机 OpenCV 4.5.4 中无法运行。
#include "core.hpp"
#include "imu.hpp"
#include "models.hpp"

#include <opencv2/dnn.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;  // 给较长的命名空间取一个短名字。

namespace {
// 匿名命名空间：这些名字只在本文件内使用。
constexpr int inputSide = 640;  // YOLO 输入画布的宽和高，单位：像素。

// [检测头][该层第几个 anchor][宽或高]，数值来自本模型的解码约定。
constexpr float anchors[3][3][2] = {
    {{10, 13}, {16, 30}, {33, 23}},
    {{30, 61}, {62, 45}, {59, 119}},
    {{116, 90}, {156, 198}, {373, 326}}
};

// 把模型的原始目标分数变成 0～1；clamp 防止 exp 的参数过大。
float sigmoid(float x) {
    return 1.f / (1.f + std::exp(-std::clamp(x, -30.f, 30.f)));
}

// 一次候选检测的所有结果。下标不是类别名字，需查各自的标签顺序。
struct Detection {
    ArmorPlate armor;       // 四个灯条端点、颜色、大小与目标分数。
    int yoloColor = -1;     // YOLO 的颜色类别下标。
    int yoloNumber = -1;    // YOLO 的编号类别下标。
    int numberClass = -1;   // 独立分类器的类别下标。
    float numberScore = 0;  // 独立分类器选中类别的分数。
};

// 原模型每个检测头输出 [1,66,H,W]，即每个 anchor 的 22 个值。
// 0..7 是 LT,LB,RB,RT 四角的原始坐标；8 是目标分数，9..21 是 13 类。
class TeamYolo {
    cv::dnn::Net net;       // OpenCV DNN 加载好的网络。
    float scoreThreshold;  // 低于这个目标分数的候选会被丢弃。

public:
    TeamYolo(const std::string& model, float score)
        : net(cv::dnn::readNetFromONNX(model)), scoreThreshold(score) {
        // 使用本机 OpenCV 的 CPU 推理后端。
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        net.enableFusion(false);  // 老版 OpenCV 对此模型的层融合不稳定。
    }

    // 输入原始视频帧；输出经过分数过滤和 NMS 后的装甲候选。
    std::vector<Detection> detect(const cv::Mat& frame, const std::string& sizeMode) {
        // 横、纵使用同一个缩放比例，避免把装甲板拉变形。
        const double scale = std::min(640.0 / frame.cols, 640.0 / frame.rows);
        const int resizedW = std::clamp(cvRound(frame.cols * scale), 1, 640);
        const int resizedH = std::clamp(cvRound(frame.rows * scale), 1, 640);

        // 先创建黑色 640×640 画布，再把缩小的原图贴在左上角。
        // 因而黑边只可能出现在右边或下边。
        cv::Mat input = cv::Mat::zeros(inputSide, inputSide, CV_8UC3);
        cv::resize(frame,
                   input(cv::Rect(0, 0, resizedW, resizedH)),
                   cv::Size(resizedW, resizedH));

        // 像素除以 255 变成 0～1；true 交换 BGR->RGB；false 表示不裁图。
        net.setInput(cv::dnn::blobFromImage(input, 1.0 / 255,
                                             cv::Size(640, 640), cv::Scalar(), true, false));

        // forward 执行网络，heads 接收大、中、小三个检测头的输出。
        std::vector<cv::Mat> heads;
        net.forward(heads, net.getUnconnectedOutLayersNames());
        if (heads.size() != 3) {
            throw std::runtime_error("Expected three YOLO heads");
        }

        // 这三个数组使用相同下标：候选 d、它的外接框、它的目标分数。
        std::vector<Detection> candidates;
        std::vector<cv::Rect> boxes;
        std::vector<float> scores;

        // level=0/1/2 分别对应 80×80、40×40、20×20 网格。
        for (int level = 0; level < 3; ++level) {
            const cv::Mat& h = heads[level];  // 引用当前检测头，不复制张量。
            const int side = 80 >> level;     // 80、40、20。
            const int stride = 8 << level;    // 8、16、32 像素/格。

            // 每格 3 个 anchor，每个 anchor 有 22 个数，所以通道数应为 66。
            if (h.type() != CV_32F || h.dims != 4 || h.size[0] != 1 ||
                h.size[1] != 66 || h.size[2] != side || h.size[3] != side) {
                throw std::runtime_error("Unexpected YOLO head shape");
            }

            for (int anchor = 0; anchor < 3; ++anchor) {
                for (int y = 0; y < side; ++y) {
                    for (int x = 0; x < side; ++x) {
                        // base 是当前 anchor 的 22 列在 66 通道中的起点。
                        const int base = anchor * 22;
                        // lambda 是临时小函数：读当前候选第 col 列的原始值。
                        auto value = [&](int col) {
                            return h.ptr<float>(0, base + col, y)[x];
                        };

                        // 第 9～12 列选颜色；第 13～21 列选编号类别。
                        int color = 9;
                        int number = 13;
                        for (int col = 10; col < 13; ++col) {
                            if (value(col) > value(color)) {
                                color = col;
                            }
                        }
                        for (int col = 14; col < 22; ++col) {
                            if (value(col) > value(number)) {
                                number = col;
                            }
                        }

                        // 第 8 列是目标分数，不是编号正确率。
                        const float score = sigmoid(value(8));
                        if (score < scoreThreshold) {
                            continue;  // 分数太低，跳过当前候选。
                        }

                        // 第 0～7 列给出四个点的 x/y 原始坐标。
                        cv::Point2f raw[4];
                        bool finite = true;
                        for (int k = 0; k < 4; ++k) {
                            // 模型画布坐标 = raw×anchor + 网格号×stride。
                            // 左上贴图没有左/上黑边，回原图只需除以 scale。
                            const float px =
                                (value(2 * k) * anchors[level][anchor][0] + x * stride) / scale;
                            const float py =
                                (value(2 * k + 1) * anchors[level][anchor][1] + y * stride) / scale;
                            raw[k] = {px, py};
                            finite &= std::isfinite(px) && std::isfinite(py);
                        }
                        if (!finite) {
                            continue;  // NaN/无穷大不能用作图像坐标。
                        }

                        // 模型输出顺序是 LT,LB,RB,RT；程序统一为 LT,RT,RB,LB。
                        Detection d;
                        d.armor.points = {raw[0], raw[3], raw[2], raw[1]};
                        const std::vector<cv::Point2f> polygon(d.armor.points.begin(),
                                                                d.armor.points.end());
                        if (!cv::isContourConvex(polygon) || cv::contourArea(polygon) < 20) {
                            continue;  // 点顺序异常或区域太小。
                        }

                        // 大部分框必须处于原始画面内。
                        const cv::Rect box = cv::boundingRect(polygon);
                        if ((box & cv::Rect(0, 0, frame.cols, frame.rows)).area() <
                            box.area() * 0.7) {
                            continue;
                        }

                        d.armor.confidence = score;
                        const std::string detectedColor = pointColor(frame, d.armor);
                        const std::string modelColor[4] = {
                            "blue", "red", "extinguish", "purple"
                        };
                        // 图像采样颜色不明确时，才退回模型的颜色类别。
                        d.armor.color = detectedColor == "unknown"
                                            ? modelColor[color - 9]
                                            : detectedColor;
                        setArmorSize(d.armor, sizeMode);
                        d.yoloColor = color - 9;
                        d.yoloNumber = number - 13;

                        // 1 号大装甲是参考同结构模型的规则，队内映射仍需核对。
                        if (sizeMode == "auto" && d.yoloNumber == 1) {
                            d.armor.size = "large";
                        }

                        candidates.push_back(d);
                        boxes.push_back(box);
                        scores.push_back(score);
                    }
                }
            }
        }

        // NMS：多个候选指向同一块板时，保留分数较高的候选。
        std::vector<int> keep;
        cv::dnn::NMSBoxes(boxes, scores, scoreThreshold, 0.4f, keep);
        std::vector<Detection> result;
        for (int i : keep) {
            result.push_back(candidates[i]);
        }
        return result;
    }
};

// 标定文件是普通文本而非 OpenCV YAML，按键读取方括号中的数字。
std::vector<double> readArray(const std::string& text,
                              const std::string& key,
                              size_t count) {
    // 先找“键名:”，再找该键后面的 [ ... ]。
    const size_t start = text.find(key + ":");
    if (start == std::string::npos) {
        throw std::runtime_error("Missing calibration key: " + key);
    }
    const size_t left = text.find('[', start);
    const size_t right = text.find(']', left);
    if (left == std::string::npos || right == std::string::npos) {
        throw std::runtime_error("Bad calibration key: " + key);
    }

    // 把方括号里的逗号换成空格，方便用 >> 逐个读取数字。
    std::string values = text.substr(left + 1, right - left - 1);
    std::replace(values.begin(), values.end(), ',', ' ');
    std::istringstream ss(values);
    std::vector<double> out;
    double n;
    while (ss >> n) {
        out.push_back(n);
    }
    if (out.size() != count) {
        throw std::runtime_error("Wrong calibration array length: " + key);
    }
    return out;
}

// 除相机内参外，还保存相机坐标到机体坐标的固定安装变换。
struct Calibration {
    Camera camera;
    cv::Matx33d bodyFromCamera;
    cv::Vec3d bodyTranslationMm;
};

Calibration readCalibration(const std::string& path) {
    // 标定文件是普通文本，先一次性读入字符串。
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open calibration: " + path);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string text = buffer.str();

    // K 有 9 个数，畸变有 5 个数，两个旋转各有 9 个数，平移有 3 个数。
    const auto k = readArray(text, "camera_matrix", 9);
    const auto dist = readArray(text, "distort_coeffs", 5);
    const auto rg = readArray(text, "R_gimbal2imubody", 9);
    const auto rc = readArray(text, "R_camera2gimbal", 9);
    const auto t = readArray(text, "t_camera2gimbal", 3);

    Calibration c;
    c.camera.K = cv::Mat(3, 3, CV_64F);
    c.camera.distortion = cv::Mat(1, 5, CV_64F);
    cv::Matx33d gimbalFromCamera;
    cv::Matx33d bodyFromGimbal;
    for (int i = 0; i < 9; ++i) {
        c.camera.K.at<double>(i / 3, i % 3) = k[i];
        gimbalFromCamera(i / 3, i % 3) = rc[i];
        bodyFromGimbal(i / 3, i % 3) = rg[i];
    }
    for (int i = 0; i < 5; ++i) {
        c.camera.distortion.at<double>(i) = dist[i];
    }

    // 先从相机转到云台，再从云台转到机体。
    c.bodyFromCamera = bodyFromGimbal * gimbalFromCamera;
    // 这里假设标定平移量以米给出，乘 1000 转毫米；需与队内记录核对。
    c.bodyTranslationMm =
        bodyFromGimbal * cv::Vec3d(t[0] * 1000, t[1] * 1000, t[2] * 1000);
    return c;
}

// 9 类分类器输出类别下标；没有训练标签映射时不能把下标猜成编号。
class TeamNumber {
    cv::dnn::Net net;  // 已加载的 9 类模型。

public:
    explicit TeamNumber(const std::string& model)
        : net(cv::dnn::readNetFromONNX(model)) {}

    // frame 是原始彩色帧；d 内已有四点，函数会填写分类下标和分数。
    void classify(const cv::Mat& frame, Detection& d, cv::Mat* debug = nullptr) {
        const auto& p = d.armor.points;
        // dst 是拉正后的四个目标角点；128×64 只是中间图尺寸。
        const cv::Point2f dst[4] = {{0, 0}, {127, 0}, {127, 63}, {0, 63}};
        cv::Mat rectified;
        cv::Mat gray;
        cv::warpPerspective(frame, rectified,
                            cv::getPerspectiveTransform(p.data(), dst),
                            cv::Size(128, 64));

        // 从拉正图中裁两灯之间的中央区域，再转成单通道灰度图。
        // ROI 的位置仍需与队内训练预处理核对。
        cv::cvtColor(rectified(cv::Rect(30, 4, 68, 56)), gray,
                     cv::COLOR_BGR2GRAY);

        // 保持宽高比，让灰度图完整放入 32×32 范围。
        const double s = std::min(32.0 / gray.cols, 32.0 / gray.rows);
        const int w = std::clamp(cvRound(gray.cols * s), 1, 32);
        const int h = std::clamp(cvRound(gray.rows * s), 1, 32);
        cv::Mat patch = cv::Mat::zeros(32, 32, CV_8U);
        // Rect 指定贴到黑画布左上角的区域；Size 指定缩放后的尺寸。
        cv::resize(gray, patch(cv::Rect(0, 0, w, h)), cv::Size(w, h));
        if (debug) {
            *debug = patch.clone();  // 调试时把真正送给模型的小图复制出去。
        }

        // 将 0～255 的像素除以 255；forward 输出 9 个类别分数。
        net.setInput(cv::dnn::blobFromImage(patch, 1.0 / 255,
                                             cv::Size(32, 32)));
        cv::Mat output = oneOutput(net).reshape(1, 1);
        if (output.total() != 9) {
            throw std::runtime_error("Number model must output nine scores");
        }

        cv::Mat probabilities;
        double minV;
        double maxV;
        cv::minMaxLoc(output, &minV, &maxV);
        // 若输出不是已经归一化的概率，就用 softmax 转换。
        if (minV < 0 || maxV > 1 || std::abs(cv::sum(output)[0] - 1) > 0.05) {
            cv::exp(output - maxV, probabilities);
            probabilities /= cv::sum(probabilities)[0];
        } else {
            probabilities = output;
        }

        // best 始终保存目前所见最大分数的下标。
        int best = 0;
        for (int i = 1; i < 9; ++i) {
            if (probabilities.at<float>(i) > probabilities.at<float>(best)) {
                best = i;
            }
        }
        d.numberClass = best;
        d.numberScore = probabilities.at<float>(best);
    }
};

std::vector<std::string> loadLabels(const std::string& path) {
    if (path.empty()) {
        return {};  // 未提供标签文件时，画面只显示类别下标。
    }
    std::ifstream f(path);  // 打开文本文件，按行读取 YOLO 的标签。
    if (!f) {
        throw std::runtime_error("Cannot open labels: " + path);
    }
    std::vector<std::string> labels;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) {
            labels.push_back(line);  // 行的顺序就是类别下标的顺序。
        }
    }
    if (labels.size() != 9) {
        throw std::runtime_error("labels file must have exactly nine lines");
    }
    return labels;
}

// 把数字转成保留 digits 位小数的字符串；不修改原数字。
std::string fmt(double value, int digits = 2) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(digits) << value;
    return s.str();
}

// frame 用引用传入，画线和文字会直接修改这一帧。
void draw(cv::Mat& frame, const Detection& d, const Pose& pose,
          const std::vector<std::string>& labels) {
    const auto& a = d.armor;  // 给装甲数据取一个短别名，不复制它。
    // Scalar 在 OpenCV 彩色图中按 B、G、R 排列。
    const cv::Scalar color = a.color == "blue" ? cv::Scalar(255, 180, 0)
                            : a.color == "red" ? cv::Scalar(0, 100, 255)
                                               : cv::Scalar(0, 255, 0);
    for (int k = 0; k < 4; ++k) {
        // %4 让最后一个点连回第一个点，形成封闭四边形。
        cv::line(frame, a.points[k], a.points[(k + 1) % 4], color, 2);
        cv::putText(frame, "P" + std::to_string(k),
                    a.points[k] + cv::Point2f(3, -4), cv::FONT_HERSHEY_SIMPLEX,
                    0.45, cv::Scalar(0, 255, 255), 1);
    }

    // 有标签时查名字；没有标签时显示 idx 加下标。
    std::string number = d.yoloNumber < 0 ? "?"
                         : labels.empty() ? "idx" + std::to_string(d.yoloNumber)
                                          : labels[d.yoloNumber];
    std::string title = a.color + " " + a.size + " " + number + " " + fmt(a.confidence);
    if (pose.valid) {
        title += " " + fmt(cv::norm(pose.tvec) / 1000) + "m";  // 毫米转米。
    }

    // 把文字起点限制在画面范围内。
    const cv::Point origin(std::clamp(cvRound(a.points[0].x), 0, frame.cols - 1),
                           std::clamp(cvRound(a.points[0].y) - 12, 20, frame.rows - 1));
    cv::putText(frame, title, origin, cv::FONT_HERSHEY_SIMPLEX, 0.55, color, 2);
}
}  // namespace

// argc 是命令行参数数量；argv 保存每个参数字符串。
int main(int argc, char** argv) {
    try {
        // 第 1 步：默认文件路径，后面的命令行参数可以覆盖它们。
        const std::string assets = "/home/travel/下载/自瞄任务素材包";
        std::string video = "/home/travel/RM_vision/task3/demo.avi";
        std::string model = "/home/travel/chatgpt/armor_extra/models/yolov5_heads_fp32.onnx";
        std::string classifier = assets + "/tiny_resnet.onnx";
        std::string calibration = assets + "/calibration.txt";
        std::string imu = assets + "/demo.txt";
        std::string output = "/home/travel/chatgpt/armor_extra/results/team_result.avi";
        std::string labelsFile = "/home/travel/chatgpt/armor_extra/config/team_yolo_labels.txt";
        std::string sizeMode = "auto";  // 每块板自动初判大/小。
        float score = 0.35f;           // YOLO 目标分数的过滤门槛。
        int limit = -1;               // -1 表示不限处理帧数。
        bool step = false;            // true 时每帧暂停等按键。
        bool show = false;            // true 时边处理边显示，不必等整个视频完成。
        bool imuAligned = false;      // 默认没有确认视频与 IMU 的起始时间。
        double imuStart = 0;          // 视频第 0 帧在 IMU 时间轴上的秒数。

        // argv[0] 是程序自身名称，所以从 argv[1] 开始读用户参数。
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            // next 是临时小函数；[&] 允许它修改外面的参数下标 i。
            auto next = [&]() -> std::string {
                if (++i >= argc) {
                    throw std::runtime_error("Missing value after " + arg);
                }
                return argv[i];  // 例如 --video 后面的视频路径。
            };

            if (arg == "--video") {
                video = next();
            } else if (arg == "--model") {
                model = next();
            } else if (arg == "--classifier") {
                classifier = next();
            } else if (arg == "--calibration") {
                calibration = next();
            } else if (arg == "--imu") {
                imu = next();
            } else if (arg == "--imu-start") {
                imuStart = std::stod(next());  // 字符串转 double 秒数。
                imuAligned = true;            // 用户提供了起始时间对应关系。
            } else if (arg == "--output") {
                output = next();
            } else if (arg == "--labels") {
                labelsFile = next();
            } else if (arg == "--type") {
                sizeMode = next();
            } else if (arg == "--score") {
                score = std::stof(next());  // 字符串转 float 分数。
            } else if (arg == "--limit") {
                limit = std::stoi(next());  // 字符串转 int 帧数。
            } else if (arg == "--step") {
                step = true;  // 这个开关没有额外参数值。
            } else if (arg == "--show") {
                show = true;  // 连续预览；同时传 --step 时，每帧暂停。
            } else if (arg == "--help") {
                std::cout << "team_armor [--video path] [--output path] [--limit frames] [--show] [--step]\n"
                          << " [--score 0.35] [--type auto|small|large] [--labels nine-line-file]\n"
                          << " [--imu-start seconds] [--model path] [--classifier path]\n";
                return 0;
            } else {
                throw std::runtime_error("Unknown option: " + arg);
            }
        }

        // 检查参数值，避免带着无效设置进入推理。
        if (sizeMode != "auto" && sizeMode != "small" && sizeMode != "large") {
            throw std::runtime_error("--type must be auto, small, or large");
        }
        if (score <= 0 || score >= 1) {
            throw std::runtime_error("--score must be in (0,1)");
        }

        // 第 2 步：打开视频，先读出第一帧，确定画面的宽高。
        cv::VideoCapture cap(video);
        if (!cap.isOpened()) {
            throw std::runtime_error("Cannot open video: " + video);
        }
        cv::Mat frame;
        if (!cap.read(frame)) {
            throw std::runtime_error("Video has no decodable frames");
        }

        // 第 3 步：资料读入内存，模型加载一次，后续各帧重复使用。
        const Calibration cal = readCalibration(calibration);
        const ImuSeries imuSeries(imu);
        const auto labels = loadLabels(labelsFile);
        TeamYolo detector(model, score);
        TeamNumber numbers(classifier);

        // 读取帧率；读不到有效值时使用 30 帧/秒。
        const double fps = cap.get(cv::CAP_PROP_FPS) > 0
                               ? cap.get(cv::CAP_PROP_FPS)
                               : 30;

        // 第 4 步：创建输出目录、视频写入器和 CSV 文件。
        fs::create_directories(fs::path(output).parent_path());
        cv::VideoWriter writer(output, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                               fps, frame.size());
        if (!writer.isOpened()) {
            throw std::runtime_error("Cannot write video: " + output);
        }
        const std::string csvPath = fs::path(output).replace_extension(".csv").string();
        std::ofstream csv(csvPath);

        // CSV 第一行是列名，后面每块候选装甲写一行。
        csv << "frame,time_s,color,size,yolo_color_index,yolo_number_index,yolo_score,"
               "classifier_index,classifier_score,p0x,p0y,p1x,p1y,p2x,p2y,p3x,p3y,"
               "camera_x_mm,camera_y_mm,camera_z_mm,body_x_mm,body_y_mm,body_z_mm,"
               "reprojection_px,imu_qx,imu_qy,imu_qz,imu_qw\n";
        if (show || step) {
            cv::namedWindow("team armor", cv::WINDOW_NORMAL);
            cv::resizeWindow("team armor", 640, 480);  // 只调整显示窗口。
        }

        int index = 0;      // 当前帧编号，从 0 开始。
        int total = 0;      // 写入结果的候选数量，不是识别准确率。
        int poseCount = 0;  // 得到有效 PnP 解的数量。

        // 第 5 步：第一帧已在 frame 里，do 先处理，再到 while 读下一帧。
        do {
            auto detections = detector.detect(frame, sizeMode);  // 整帧找板。
            const double time = index / fps;  // 当前帧相对视频开头的秒数。
            Quaternion q;                    // 接收当前时刻的四元数。
            // 未确认对时就不查询；两条 IMU 采样间隔不得超过 0.2 秒。
            const bool hasImu =
                imuAligned && imuSeries.at(imuStart + time, q, 0.2);

            // auto& 让 d 直接引用列表中的当前候选，分类结果可写回其中。
            for (auto& d : detections) {
                numbers.classify(frame, d);  // 独立分类器看候选的中间图案。
                if (!labels.empty() && labels[d.yoloNumber] == "not_armor") {
                    continue;  // 此过滤依据 YOLO 标签，不是独立分类器下标。
                }
                const Pose pose = estimatePose(d.armor, cal.camera);
                if (pose.valid) {
                    ++poseCount;
                }
                ++total;
                draw(frame, d, pose, labels);  // 在 frame 上画框、编号、距离。

                // 先写帧号、时间、颜色、大小和两个模型的诊断结果。
                csv << index << ',' << fmt(time, 3) << ',' << d.armor.color << ','
                    << d.armor.size << ',' << d.yoloColor << ',' << d.yoloNumber << ','
                    << fmt(d.armor.confidence, 4) << ',' << d.numberClass << ','
                    << fmt(d.numberScore, 4);
                for (const auto& point : d.armor.points) {
                    csv << ',' << fmt(point.x, 2) << ',' << fmt(point.y, 2);
                }

                if (pose.valid) {
                    // 用标定中的固定安装旋转和平移，把相机位置换到机体。
                    // 云台运动时还需当时的云台角度；此处不计算世界坐标。
                    const cv::Vec3d body =
                        cal.bodyFromCamera * pose.tvec + cal.bodyTranslationMm;
                    for (int j = 0; j < 3; ++j) {
                        csv << ',' << fmt(pose.tvec[j], 2);  // 相机 xyz，毫米。
                    }
                    for (int j = 0; j < 3; ++j) {
                        csv << ',' << fmt(body[j], 2);       // 机体 xyz，毫米。
                    }
                    csv << ',' << fmt(pose.errorPx, 3);      // 重投影误差，像素。
                } else {
                    csv << ",,,,,,,";  // 无有效解时，七列留空。
                }

                if (hasImu) {
                    csv << ',' << q.x << ',' << q.y << ',' << q.z << ',' << q.w;
                } else {
                    csv << ",,,,";  // 没有对齐的 IMU 数据，四列留空。
                }
                csv << '\n';  // 结束当前候选这一行。
            }

            // 不论这一帧有没有检测到板，都保存画面。
            writer.write(frame);
            ++index;  // 已保存的这一帧也要计数，即使随后按键退出。
            if (show || step) {
                // 单独生成缩小的预览图；frame 仍保留原始尺寸用于检测和保存。
                const double previewScale =
                    std::min(1.0, std::min(640.0 / frame.cols, 480.0 / frame.rows));
                cv::Mat preview;
                cv::resize(frame, preview,
                           cv::Size(cvRound(frame.cols * previewScale),
                                    cvRound(frame.rows * previewScale)),
                           0, 0, cv::INTER_AREA);
                cv::imshow("team armor", preview);
                // 0：一直等按键；1：短暂等待并处理窗口事件，然后继续下一帧。
                const int key = cv::waitKey(step ? 0 : 1) & 0xff;
                if (key == 27 || key == 'q') {
                    break;  // Esc 或 q 退出，已处理的画面仍保存在输出视频中。
                }
            }

            if (index % 30 == 0) {
                std::cerr << "\rprocessed " << index << " frames, " << total
                          << " detections" << std::flush;
            }
            if (limit >= 0 && index >= limit) {
                break;  // 到达用户设定的处理帧数。
            }
        } while (cap.read(frame));  // 读取下一帧；读不到就自然结束。

        if (show || step) {
            cv::destroyAllWindows();
        }
        std::cerr << "\nframes=" << index << " detections=" << total << " poses=" << poseCount
                  << "\nvideo: " << output << "\ncsv: " << csvPath << '\n';
        if (!imuAligned) {
            std::cerr << "IMU timestamps are not aligned to this video. "
                         "Use --imu-start only after checking synchronization.\n";
        }
        return 0;  // 离开作用域时，文件流与视频对象自动释放资源。
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error: " << e.what() << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
