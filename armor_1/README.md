# 自瞄加试题：实际素材版 C++ 程序

当前入口是 `team_armor.cpp`，编译后的程序是 `build/team_armor`。
早期教学版 `main.cpp`、独立测试文件 `geometry_test.cpp` 和旧源码压缩包已按用户要求删除。

## 编译与运行

```bash
cd /home/travel/chatgpt/armor_extra
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
./build/team_armor --help
./build/team_armor --show
./build/team_armor --step
```

默认读取 task3/demo.avi、素材包中的分类器/标定/IMU，以及 models/yolov5_heads_fp32.onnx。
`--show` 连续显示当前处理结果，同时保存视频和 CSV；显示速度取决于推理速度，不保证实时。
预览窗口中 Esc 或 q 退出，输出保留已处理的部分。
不传 `--show` 或 `--step` 时只保存结果，不打开窗口。
逐帧模式下 Esc 或 q 退出，其余按键前进。运行参数与实际验证情况见 [TEAM_SOLUTION.md](TEAM_SOLUTION.md)。

## 源码阅读顺序

1. `team_armor.cpp` 的 `main()`：视频、模型、循环、输出。
2. `core.hpp`：四点、大小判断和 PnP。
3. `imu.hpp`：四元数、时间戳和姿态插值。
4. 回到 `team_armor.cpp` 阅读 `TeamYolo` 与 `TeamNumber`。

详细入门解释见 [从主循环读懂自瞄代码.md](从主循环读懂自瞄代码.md)。
分类器标签、标定单位、IMU 对时等尚需核对的事项见 TEAM_SOLUTION.md。
