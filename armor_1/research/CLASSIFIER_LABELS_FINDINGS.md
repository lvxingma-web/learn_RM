# 独立分类器下标核对记录

核对日期：2026-10-04。学校组织：https://github.com/orgs/typical-motion/repositories

## 结论

学校 FJUTvision_26 当前自瞄代码的标签映射可以确定；素材包 tiny_resnet.onnx 是否沿用它，只能作为待验证推断，不能直接确认。

## 学校仓库的明确映射

| 下标 | label.txt 内容 | 含义 |
|---|---|---|
| 0 | 1 | 1 号 |
| 1 | 2 | 2 号 |
| 2 | 3 | 3 号 |
| 3 | 4 | 4 号 |
| 4 | 5 | 5 号 |
| 5 | outpost | 前哨站 |
| 6 | sentry | 哨兵 |
| 7 | base | 基地 |
| 8 | negative | 非装甲/负样本 |

源码依据：

- https://github.com/typical-motion/FJUTvision_26/blob/master/src/rm_auto_aim/armor_detector/model/label.txt
- number_classifier.cpp 将标签文件逐行 push_back 到 class_names_，取模型输出最大值下标 label_id，再用 class_names_[label_id] 查询名称。
- armor_detector_node.cpp 加载 model/lenet.onnx 和 model/label.txt。
- 本地目录：/home/travel/FJUTvision_26
- 本地及远端 master 核对到的提交：d5d0cd7f0b32bcb0134374d5aa024f87454fcc7a。

## 模型是否对应

| 项目 | 学校当前 lenet.onnx | 任务素材 tiny_resnet.onnx |
|---|---|---|
| 输入 | 1×1×28×28 | batch×1×32×32 |
| 输出 | 1×9 | batch×9 |
| 网络 | LeNet，2 个卷积节点 | 残差网络，22 个卷积节点、9 个 Add 节点 |
| 输出图末端 | 有 Softmax | 没有 Softmax |
| 标签元数据 | 未附带 | 未附带 |
| 文件 SHA256 | 21278d220f0c19bf6b497b8052dabb8a4ad1a82fe516b786decff0eb760a4a75 | 9f45670da9e61f31b60c12efb08c8a7522ff56cc220c7fa9907283ce7e730179 |

学校当前预处理：透视变换 → 中央 20×28 ROI → 灰度 → Otsu 二值化 → 缩放到 28×28 → 除以 255。
任务素材协议：灰度 32×32、保持比例贴左上。两个模型的输入和预处理不同，不能互换。

学校当前仓库文件树未找到 tiny_resnet.onnx 或其训练标签映射。素材文件只包含权重和结构，没有类别名称。九分类输出数量与“非装甲”常见放末尾只能作为线索，不能证明全部下标与 label.txt 一致。

如 tiny_resnet 的训练沿用学校 label.txt，则上表可以沿用，classifier_index=8 表示 negative；确认方法是找到该模型训练时的 class_to_idx/labels 或与它明确配套的推理源码。不能用输出多数为 8 本身证明 8 的含义。

## 与当前 YOLO 的区别

当前教学代码 YOLO 标签表为 sentry, one, two, three, four, five, outpost, base, not_armor（参考公开同结构模型，仍需和任务 YOLO 训练核对）。因此 YOLO 下标 0 当前被解释为哨兵；学校分类器下标 0 对应 1 号。两者不能混用标签表。

本次不修改程序的 tiny_resnet 下标解释，也不据此过滤候选。

