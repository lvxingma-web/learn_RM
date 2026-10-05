#pragma once
#include "core.hpp"
#include <opencv2/dnn.hpp>
#include <stdexcept>

inline std::vector<std::string> classNames(const cv::FileStorage& f) {
    std::vector<std::string> labels;
    f["labels"] >> labels;
    if (labels.size() != 9) throw std::runtime_error("Model config needs 9 labels in training order");
    for (const auto& s : labels)
        if (s.empty() || s.find("TODO") == 0)
            throw std::runtime_error("Replace TODO labels with actual training order");
    return labels;
}
inline float probability(float value, bool logits) {
    if (!std::isfinite(value)) throw std::runtime_error("Model produced non-finite scores");
    if (logits) return 1.0f / (1.0f + std::exp(-value));
    if (value < 0 || value > 1.0001f)
        throw std::runtime_error("Scores are not probabilities; check model output activation");
    return value;
}
inline cv::Mat oneOutput(cv::dnn::Net& net) {
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());
    if (outputs.size() != 1 || outputs[0].type() != CV_32F)
        throw std::runtime_error("This adapter expects one float32 output; inspect team inference code");
    return outputs[0].clone();
}

// 单独的 32x32 灰度数字分类器。这里只实现一个明确的参考预处理协议。
// 必须用队内训练/推理代码核对裁剪比例、归一化、阈值和类别顺序。
class NumberClassifier {
    cv::dnn::Net net;
    std::vector<std::string> labels;
    int negative;
    double scale, confidence, left, right, top, bottom;
    bool logits;
public:
    NumberClassifier(const std::string& model, const std::string& config) {
        cv::FileStorage f(config, cv::FileStorage::READ);
        if (!f.isOpened()) throw std::runtime_error("Cannot read classifier config");
        labels = classNames(f);
        negative = (int)f["negative_class_index"];
        scale = (double)f["input_scale"];
        confidence = (double)f["confidence_threshold"];
        left = (double)f["roi_left"]; right = (double)f["roi_right"];
        top = (double)f["roi_top"]; bottom = (double)f["roi_bottom"];
        logits = (int)f["output_logits"] != 0;
        if (negative < 0 || negative >= 9 || scale <= 0 || confidence <= 0 || confidence > 1 ||
            left < 0 || right > 1 || left >= right || top < 0 || bottom > 1 || top >= bottom)
            throw std::runtime_error("Fill classifier config from team preprocessing; placeholders are invalid");
        net = cv::dnn::readNetFromONNX(model);
    }
    // 返回 false 表示“不是装甲板”。低置信度保留候选，但编号记 unknown。
    bool classify(const cv::Mat& frame, ArmorPlate& a, cv::Mat& grayInput) {
        const int height = 64;
        const int width = cvRound(height * (a.size == "large" ? 230.0 : 135.0) / 55.0);
        const std::array<cv::Point2f,4> dst{{{0,0}, {(float)width-1,0},
                   {(float)width-1,(float)height-1}, {0,(float)height-1}}};
        cv::Mat rectified;
        cv::warpPerspective(frame, rectified,
            cv::getPerspectiveTransform(a.points.data(), dst.data()), cv::Size(width,height));
        const cv::Rect roi(cvRound(width*left), cvRound(height*top),
            std::max(1,cvRound(width*(right-left))), std::max(1,cvRound(height*(bottom-top))));
        cv::Mat gray;
        cv::cvtColor(rectified(roi & cv::Rect(0,0,width,height)), gray, cv::COLOR_BGR2GRAY);
        const double s = std::min(32.0/gray.cols, 32.0/gray.rows);
        const int w = std::clamp(cvRound(gray.cols*s),1,32);
        const int h = std::clamp(cvRound(gray.rows*s),1,32);
        grayInput = cv::Mat::zeros(32,32,CV_8U);
        cv::resize(gray, grayInput(cv::Rect(0,0,w,h)), cv::Size(w,h)); // 贴左上，右下补零
        net.setInput(cv::dnn::blobFromImage(grayInput, scale, cv::Size(32,32)));
        cv::Mat out = oneOutput(net).reshape(1,1);
        if (out.total() != 9) throw std::runtime_error("Classifier must produce exactly 9 scores");
        if (logits) { // 多分类 logits 转 softmax；不把最大 logit 当置信度
            double maxValue;
            cv::minMaxLoc(out, nullptr, &maxValue);
            cv::exp(out-maxValue, out);
            out /= cv::sum(out)[0];
        }
        int best=0;
        for (int k=0; k<9; ++k) {
            probability(out.at<float>(k),false);
            if (out.at<float>(k)>out.at<float>(best)) best=k;
        }
        a.confidence = out.at<float>(best);
        a.number = a.confidence >= confidence ? labels[best] : "unknown";
        return a.confidence < confidence || best != negative;
    }
};

// 自定义 YOLO 22 列输出适配器：字段索引全部来自配置，不猜测队内模型布局。
// 此适配器用左上对齐的 letterbox，只在右/下补黑边，因此角点只除缩放。
class YoloAdapter {
    cv::dnn::Net net;
    std::vector<std::string> labels;
    std::vector<int> classes, points;
    int inputW, inputH, objectness, negative;
    double scale, threshold, nms;
    bool swapRB, scoreLogits;
    std::string layout, units;
public:
    YoloAdapter(const std::string& model, const std::string& config) {
        cv::FileStorage f(config,cv::FileStorage::READ);
        if (!f.isOpened()) throw std::runtime_error("Cannot read YOLO contract");
        labels = classNames(f);
        f["class_indices"] >> classes;
        f["keypoint_indices"] >> points;
        inputW=(int)f["input_width"]; inputH=(int)f["input_height"];
        objectness=(int)f["objectness_index"]; negative=(int)f["negative_class_index"];
        scale=(double)f["input_scale"]; threshold=(double)f["confidence_threshold"];
        nms=(double)f["nms_threshold"]; swapRB=(int)f["swap_rb"]!=0;
        scoreLogits=(int)f["scores_are_logits"]!=0;
        f["output_layout"] >> layout; f["keypoint_units"] >> units;
        if (inputW<=0 || inputH<=0 || scale<=0 || classes.size()!=9 || points.size()!=8 ||
            negative<0 || negative>=9 || objectness < -1 || objectness>=22 ||
            threshold<=0 || threshold>1 || nms<=0 || nms>1 ||
            (layout!="rows" && layout!="channels") || (units!="pixels" && units!="normalized"))
            throw std::runtime_error("Fill YOLO contract from team decoder; example is intentionally invalid");
        std::vector<int> used = classes;
        used.insert(used.end(),points.begin(),points.end());
        if (objectness>=0) used.push_back(objectness);
        std::sort(used.begin(),used.end());
        if (used.front()<0 || used.back()>=22 || std::adjacent_find(used.begin(),used.end())!=used.end())
            throw std::runtime_error("YOLO indices must be valid, distinct columns in [0,21]");
        net=cv::dnn::readNetFromONNX(model);
    }
    std::vector<ArmorPlate> detect(const cv::Mat& frame,const std::string& sizeMode) {
        const double resizeScale=std::min((double)inputW/frame.cols,(double)inputH/frame.rows);
        cv::Mat input=cv::Mat::zeros(inputH,inputW,CV_8UC3);
        const int w=std::clamp(cvRound(frame.cols*resizeScale),1,inputW);
        const int h=std::clamp(cvRound(frame.rows*resizeScale),1,inputH);
        cv::resize(frame,input(cv::Rect(0,0,w,h)),cv::Size(w,h));
        net.setInput(cv::dnn::blobFromImage(input,scale,cv::Size(inputW,inputH),cv::Scalar(),swapRB));
        cv::Mat out=oneOutput(net), rows;
        if (out.dims<2 || out.total()%22!=0)
            throw std::runtime_error("Unexpected YOLO tensor: expected 22 values per candidate");
        if (layout=="rows") {
            if (out.size[out.dims-1]!=22) throw std::runtime_error("Expected [...,N,22] output");
            rows=out.reshape(1,(int)out.total()/22);
        } else {
            if (out.size[out.dims-2]!=22) throw std::runtime_error("Expected [...,22,N] output");
            rows=out.reshape(1,22).t();
        }
        std::vector<ArmorPlate> candidates;
        std::vector<cv::Rect> boxes;
        std::vector<float> scores;
        for (int row=0; row<rows.rows; ++row) {
            const float* v=rows.ptr<float>(row);
            int best=0;
            for (int k=1; k<9; ++k) if (v[classes[k]]>v[classes[best]]) best=k;
            const float score=probability(v[classes[best]],scoreLogits) *
                (objectness>=0 ? probability(v[objectness],scoreLogits) : 1.0f);
            if (score<threshold || best==negative) continue;
            ArmorPlate a;
            bool finite=true;
            for (int k=0;k<4;++k) {
                double x=v[points[k*2]], y=v[points[k*2+1]];
                if (units=="normalized") {x*=inputW; y*=inputH;}
                a.points[k]=cv::Point2f(x/resizeScale,y/resizeScale); // 不减黑边
                finite &= std::isfinite(x) && std::isfinite(y);
            }
            const std::vector<cv::Point2f> polygon(a.points.begin(),a.points.end());
            if (!finite || !cv::isContourConvex(polygon) || std::abs(cv::contourArea(polygon))<1)
                continue;
            a.number=labels[best]; a.confidence=score;
            a.color=pointColor(frame,a);
            setArmorSize(a,sizeMode);
            candidates.push_back(a);
            boxes.push_back(cv::boundingRect(polygon)); scores.push_back(score);
        }
        std::vector<int> keep;
        cv::dnn::NMSBoxes(boxes,scores,(float)threshold,(float)nms,keep);
        std::vector<ArmorPlate> result;
        for (int index:keep) result.push_back(candidates[index]);
        return result;
    }
};
