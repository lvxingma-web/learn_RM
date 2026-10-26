# 模板初阶 / STL / string 练习

C++ 模板与 string 的练习工程，对应讲义 **06 模板初阶**、**07 STL 简介**、**08 string**。

## 目录

```
templete_and_string/
├── 模板STL_string_练习册.md    # 14 道练习 + 知识点对照 + 参考代码
├── code/                       # 可直接编译运行的配套代码
│   ├── 01_template_basics.cpp  # 函数模板 Swap/Max、重载匹配规则
│   ├── 02_stack.cpp            # 类模板 Stack<T>，类外定义成员
│   ├── 03_array.cpp            # 类模板 Array<T>，begin/end 与范围 for
│   ├── 04_mystring.cpp         # 深拷贝：传统版 / 现代版
│   └── 05_list_node.cpp        # 链表节点模板 + 带头单链表
├── templete_and_string.sln     # Visual Studio 工程
└── templete_and_string/
    ├── templete_and_string.vcxproj
    └── test.cpp                # 平时练习代码
```

## 编译

命令行：

```bash
g++ -std=c++17 code/01_template_basics.cpp -o test
```

Visual Studio：新建空项目，把 `code/` 下的 `.cpp` 添加进去即可
（中文注释若报 C4819，项目属性 → C/C++ → 命令行加 `/utf-8`）。

## 练习册内容

| 部分 | 题号 | 主题 |
|---|---|---|
| 模板初阶 | 1–4 | 函数模板、匹配规则、`Stack<T>`、`Array<T>` |
| STL 简介 | 5–6 | 六大组件、迭代器分类 |
| string | 7–14 | 6 道 OJ、`MyString` 模拟实现、`auto` |
