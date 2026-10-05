# 加试 1、2：素材包模型与视频实测版

本目录的 `team_armor.cpp` 是独立可执行程序，不会改动 `task3/demo.avi` 或素材包原件。输入默认指向这台电脑上用户给的文件，输出写到 `armor_extra/results/`。

## 编译与运行

在终端执行：

```bash
cd /home/travel/chatgpt/armor_extra
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target team_armor -j4
./build/team_armor --output /home/travel/chatgpt/armor_extra/results/team_result_final.avi
```

生成同名 `.avi`（逐帧画框）和 `.csv`（角点、类别、相机与机体坐标）。先试前 100 帧可加 `--limit 100`。逐帧检查加 `--step`，在窗口按空格下一帧，Esc 或 q 退出。只想检测红/蓝装甲时，可以在结果中依据颜色列筛选；程序保留所有模型候选供调试。

运行时的完整参数见 `./build/team_armor --help`。`--type auto` 根据灯条间距/高度判断大小；某块板实际尺寸已经确定时，可以临时用 `--type small` 或 `--type large` 检查 PnP。输出视频里的 `0.87m` 是相机到装甲的距离，CSV 的 `camera_*_mm` 是相机坐标，`body_*_mm` 是把相机坐标经过标定旋转、平移换算到 IMU 机体坐标，数值单位为毫米。

## 每一阶段做什么

1. 视频原帧按比例缩到 640×640 的左上角，剩下的右边或下边填黑；BGR 转 RGB，再除以 255。因为左上角没有填充，模型角点回到原视频只需除缩放比例。
2. 模型三路输出分别对应步长 8、16、32。每个网格点有三个 anchor，每个 anchor 有 22 个数。原模型中，角点的后处理是 `原始预测 × anchor + 网格位置 × 步长`。四角原顺序是左上、左下、右下、右上；程序转成题目要求的左上、右上、右下、左下。第 8 列经 sigmoid 得到目标分数，第 9～12 列选颜色，第 13～21 列选编号，之后使用 NMS 合并重叠框。
3. 候选框按灯条端点求大小，用标定文件中的相机内参和畸变系数做 IPPE PnP。135×55 mm 对应小装甲，230×55 mm 对应大装甲；CSV 输出相机前的 xyz 和重投影误差。
4. `tiny_resnet.onnx` 另做一次灰度 32×32 左上贴图分类，结果在 CSV 中 `classifier_index` 和 `classifier_score`。在本视频中当前 ROI 的分类多数给出下标 8；独立分类器的训练标签顺序尚未核对，不能直接把该下标解释为“不是装甲板”。它只作诊断，不参与过滤或更改 YOLO 的编号；预处理与画面清晰度仍需核对，不能用它假称编号已被双模型确认。

默认的 `config/team_yolo_labels.txt` 参考[公开同结构 YOLO 推理代码](https://github.com/TongjiSuperPower/sp_vision_25/blob/main/tasks/auto_aim/yolos/yolov5.cpp)和[类别定义](https://github.com/TongjiSuperPower/sp_vision_25/blob/main/tasks/auto_aim/armor.hpp)，对应 `sentry, one, two, three, four, five, outpost, base, not_armor`。这些名称**不是模型文件自带的元数据**；需要和这份模型的训练配置或队内标签表核对。如果顺序不同，复制并编辑一个九行文件，用 `--labels 文件路径` 传入。CSV 始终保留原始类别下标，便于核对。

## 本机模型为何需要转换

素材包 `yolov5.onnx` 输入为 FP16，系统 OpenCV 4.5.4 不能直接运行。`tools/extract_yolo_heads.py` 把 FP16 权重转成 FP32，并暴露三个原始检测头，绕过本机 OpenCV 对原模型五维后处理张量的运行错误，输出 `models/yolov5_heads_fp32.onnx`。程序在 C++ 中完成相同的角点变换。若换模型，必须重新核对每列意义及 anchor；不可直接套用该解码器。可用下面的命令从原件重新生成：

```bash
python3 tools/extract_yolo_heads.py '/home/travel/下载/自瞄任务素材包/yolov5.onnx' models/yolov5_heads_fp32.onnx
```

## IMU 同步边界

`demo.txt` 每行是 `时间戳 qx qy qz qw`，程序按 xyzw 读取。该文件 620 行、时间从约 0.75 到 349.69 秒，中间存在 25 秒、172 秒等断档；视频只有约 22.9 秒，素材没有说明视频起始时间。因此默认只输出相机和机体坐标，IMU 列留空。若确认视频第一帧对应 IMU 时间 `T`，传 `--imu-start T`，程序仅在相邻采样间隔不超过 0.2 秒时插值并写入四元数。未确认四元数的坐标方向和同步前，也不输出所谓“世界坐标”。

## 实际验证与限制

已在本机用提供的视频、模型、标定编译运行，删除独立测试文件前，`ctest` 中端点/PnP 测试通过。整段视频顺序读取 687 帧，保存 868 条检测和 868 条有正深度的 PnP 结果；这只是**输出数量，不是准确率**。抽查第 100、200、400、500、600、680 帧，近景和多目标画面的灯条角点基本贴合；第 500、680 帧的远距离、遮挡候选值得进一步人工核验。检测和 PnP 是真实模型输出，但“精准”需要人工标注真值后算误检率、漏检率和角点误差。特别要看远处、遮挡、过曝和倾斜帧。大小分类依据宽高比及公开同结构模型的 1 号大装甲规则，透视严重时仍可能判错，距离会随板宽改变。标定文件未写相机图像尺寸、标定采集条件和平移单位，当前把平移解释成米；如果标定来自不同分辨率或单位，三维距离不可信。输出用于课程演示和调试，不应用于实车发射控制。
