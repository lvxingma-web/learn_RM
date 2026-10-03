#pragma once

#include "core.hpp"

#include <fstream>
#include <sstream>

// 文本格式：timestamp_s qx qy qz qw；或仅一行 qx qy qz qw 表示固定姿态。
// 时间戳与视频要对齐，不能按行号直接当帧号。
struct Quaternion {
    // x/y/z/w 合起来表示旋转；它们不是三个位置加一个角度。
    double x = 0;
    double y = 0;
    double z = 0;
    double w = 1;  // 默认 (0,0,0,1) 表示单位旋转。

    void normalize() {
        const double n = std::sqrt(x * x + y * y + z * z + w * w);
        if (!std::isfinite(n) || n < 1e-12) {
            throw std::runtime_error("Invalid IMU quaternion");
        }
        // 单位四元数长度为 1；每个分量除以原长度。
        x /= n;
        y /= n;
        z /= n;
        w /= n;
    }

    // 单位四元数转换为旋转矩阵；实际用于哪两个坐标系需核对队内约定。
    cv::Matx33d rotation() const {
        return {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w),
                2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w),
                2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)};
    }
};

// 在姿态 a 和 b 之间估计姿态；fraction=0 在 a，fraction=1 在 b。
inline Quaternion slerp(Quaternion a, Quaternion b, double fraction) {
    double dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    // q 和 -q 表示同一个旋转；调整符号，使插值走较短路径。
    if (dot < 0) {
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        b.w = -b.w;
        dot = -dot;
    }

    double u = 1 - fraction;  // a 的权重。
    double v = fraction;      // b 的权重。
    if (dot < 0.9995) {
        // 姿态差不小时采用球面插值；非常接近时使用上面的线性权重。
        const double theta = std::acos(std::clamp(dot, -1.0, 1.0));
        u = std::sin((1 - fraction) * theta) / std::sin(theta);
        v = std::sin(fraction * theta) / std::sin(theta);
    }
    Quaternion q{u * a.x + v * b.x, u * a.y + v * b.y,
                 u * a.z + v * b.z, u * a.w + v * b.w};
    q.normalize();  // 插值结果重新调整成单位四元数。
    return q;
}

class ImuSeries {
    // pair.first 是秒数，pair.second 是该时刻的四元数。
    std::vector<std::pair<double, Quaternion>> samples;
    bool constant = false;  // 是否只有一个固定姿态，没有时间序列。

public:
    explicit ImuSeries(const std::string& path) {
        std::ifstream in(path);  // 构造时读取整份 IMU 文件。
        if (!in) {
            throw std::runtime_error("Cannot open IMU text: " + path);
        }
        std::string line;
        int lineNumber = 0;
        while (std::getline(in, line)) {
            ++lineNumber;
            // 跳过空行、# 注释和可能存在的表头。
            if (line.empty() || line[0] == '#' || line.find("timestamp_s") == 0 ||
                line.find("qx") == 0) {
                continue;
            }
            std::replace(line.begin(), line.end(), ',', ' ');  // 兼容逗号分隔。
            std::istringstream row(line);  // 把当前这一行当作输入流。
            std::vector<double> values;
            double value;
            while (row >> value) {
                values.push_back(value);  // 逐个读取数字。
            }
            if (!row.eof() || (values.size() != 4 && values.size() != 5)) {
                throw std::runtime_error("IMU row " + std::to_string(lineNumber) +
                                         ": expected xyzw or timestamp_s xyzw");
            }
            if (!samples.empty() && (constant || values.size() == 4)) {
                throw std::runtime_error("Four-value IMU format permits exactly one quaternion");
            }

            constant = values.size() == 4;
            const int first = constant ? 0 : 1;  // 四元数起始列：有时间戳则跳过第 0 列。
            const double time = constant ? 0 : values[0];
            if (!std::isfinite(time) ||
                (!samples.empty() && time <= samples.back().first)) {
                throw std::runtime_error("IMU timestamps must be finite and strictly increasing");
            }
            Quaternion q{values[first], values[first + 1],
                         values[first + 2], values[first + 3]};  // 按 xyzw 读入。
            q.normalize();
            samples.push_back({time, q});  // 保存“时间+姿态”这一对数据。
        }
        if (samples.empty()) {
            throw std::runtime_error("IMU text has no samples");
        }
    }

    // 查询某个秒数的姿态，成功时通过引用 q 写回结果；bool 表示是否可用。
    bool at(double time, Quaternion& q,
            double maximumGap = std::numeric_limits<double>::infinity()) const {
        if (constant) {
            q = samples.front().second;  // 固定姿态对所有查询时间相同。
            return true;
        }
        if (time < samples.front().first || time > samples.back().first) {
            return false;  // 不对已记录时间范围之外的时刻进行猜测。
        }

        // upper_bound 找到第一条时间严格大于查询时间的记录。
        auto upper = std::upper_bound(samples.begin(), samples.end(), time,
                                      [](double t, const auto& sample) {
                                          return t < sample.first;
                                      });
        if (upper == samples.end()) {
            q = samples.back().second;  // 查询落在最后一条记录的时间上。
            return true;
        }
        const auto lower = upper - 1;  // 前一条记录与 upper 夹住查询时刻。
        if (upper->first - lower->first > maximumGap) {
            return false;  // 采样断档太大，不进行插值。
        }
        // 计算查询时刻在前后两条记录之间的比例，再对姿态做插值。
        q = slerp(lower->second, upper->second,
                  (time - lower->first) / (upper->first - lower->first));
        return true;
    }
};

// 以下外参读取供早期教学版使用，实测版在 team_armor.cpp 中读安装变换。
struct Extrinsic {
    cv::Matx33d imuFromCamera;  // 把相机方向旋转到 IMU 坐标系。
    cv::Vec3d translationMm;   // 相机原点在 IMU 坐标系中的位置，mm。
};

inline Extrinsic loadExtrinsic(const std::string& path) {
    cv::FileStorage f(path, cv::FileStorage::READ);
    if (!f.isOpened()) {
        throw std::runtime_error("Cannot read extrinsic file");
    }
    cv::Mat r;
    cv::Mat t;
    f["R_imu_camera"] >> r;
    f["t_imu_camera_mm"] >> t;
    if (r.rows != 3 || r.cols != 3 || t.total() != 3) {
        throw std::runtime_error("Extrinsics need R_imu_camera and t_imu_camera_mm");
    }
    r.convertTo(r, CV_64F);
    t = t.reshape(1, 3);
    t.convertTo(t, CV_64F);
    // 合法旋转矩阵要求 R×R转置≈I，且行列式≈+1。
    if (!cv::checkRange(r) || !cv::checkRange(t) ||
        cv::norm(r * r.t() - cv::Mat::eye(3, 3, CV_64F)) > 1e-4 ||
        std::abs(cv::determinant(r) - 1) > 1e-4) {
        throw std::runtime_error("Invalid extrinsic rotation: must be orthonormal with determinant +1");
    }
    Extrinsic e;
    for (int i = 0; i < 3; ++i) {
        e.translationMm[i] = t.at<double>(i);
        for (int j = 0; j < 3; ++j) {
            e.imuFromCamera(i, j) = r.at<double>(i, j);
        }
    }
    return e;
}
