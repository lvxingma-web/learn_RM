# 模板 / STL / string 三讲配套练习册

> 对应讲义：`06.模板初阶`、`07.STL简介`、`08.string`
> 用法：每题先自己写 20 分钟，卡住了再看「参考代码」。答案不是重点，**能不看讲义默写出来**才是。

## 环境

- VS2022：新建「空项目」→ 添加 `.cpp` 源文件即可（VS 默认支持 C++14 以上）。
- 或命令行：`g++ -std=c++11 test.cpp -o test`（Windows 上可用 MinGW / WSL）。
- string 模拟实现那题建议加 `-fsanitize=address`（VS 用 Debug 模式），能直接抓到重复释放。

**两个 VS 常见坑**

1. 代码里有中文注释/中文字符串时，若报 `C4819` / `C2001: 常量中有换行符`，是文件编码问题：
   文件 → 另存为 → 编码选「UTF-8 带签名」，或在项目属性 → C/C++ → 命令行加 `/utf-8`。
   本册所有代码都用 MSVC 2022 + `/std:c++17 /utf-8` 实际编译运行过。
2. 报 `C4996 'strcpy': This function or variable may be unsafe` 是警告不是错误，练习 13 直接忽略即可
   （或在文件第一行加 `#define _CRT_SECURE_NO_WARNINGS`）。

## 建议节奏（3 天）

| 天 | 内容 | 题号 |
|---|---|---|
| Day 1 | 模板：函数模板 + 类模板 | 1、2、3 |
| Day 2 | 模板进阶 + STL 初体验 | 4、5、6 |
| Day 3 | string 接口刷题 + 模拟实现 | 7～13 |

---

## 知识点 → 题目对照表

| 讲义 | 知识点 | 对应题 |
|---|---|---|
| 06 P2-P4 | 函数模板格式、隐式/显式实例化、推导冲突 | 1、2 |
| 06 P4-P5 | 模板与非模板的匹配原则、模板不做隐式转换 | 2 |
| 06 P5-P6 | 类模板格式、类外定义成员、`Stack<int>` 才是类型 | 3 |
| 06 全篇 | 模板 = 蓝本，编译器按实参生成代码 | 4 |
| 07 P1 | 六大组件：容器/算法/迭代器/仿函数/适配器/空间配置器 | 5 |
| 07 P2-P3 | 「能用 → 明理 → 能扩展」 | 5、6、13 |
| 08 P1-P3 | `auto`、范围 for | 4、14 |
| 08 P4 | 构造、size/length/capacity/empty/clear/reserve/resize | 13、14 |
| 08 P5 | `operator[]`、迭代器、find/npos/rfind/substr | 7～12 |
| 08 P5 | `+=`/`push_back`/`append`/`c_str`/`getline` | 9、13 |
| 08 P7-P10 | 牛刀小试 + 课后作业（6 道 OJ） | 7～12 |
| 08 P10-P14 | 浅拷贝/深拷贝、传统版与现代版、写时拷贝 | 13 |

---

# 第一部分：模板初阶（06）

## 练习 1　通用 Swap + Max　★

**要求**

1. 用函数模板实现 `Swap(T& a, T& b)`，不要用 `std::swap`。
2. 用函数模板实现 `Max(const T& a, const T& b)`，返回较大者的**引用**（体会为什么返回引用）。
3. 用 `int`、`double`、`std::string`，以及一个自己写的 `Date`（带 `operator<`）各测一遍。
4. 思考题：如果把 `Max` 的返回值改成 `T`，会多发生几次拷贝？

**要练的点**：函数模板定义格式、`template<class T>` 中 `class` 与 `typename` 等价（讲义 P3）、模板按类型生成多份代码。

**参考代码**

```cpp
#include <iostream>
#include <string>
using namespace std;

template<class T>
void Swap(T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

template<class T>
const T& Max(const T& a, const T& b)
{
    return a < b ? b : a;
}

struct Date
{
    int y, m, d;
    bool operator<(const Date& o) const
    {
        if (y != o.y) return y < o.y;
        if (m != o.m) return m < o.m;
        return d < o.d;
    }
};

int main()
{
    int a = 1, b = 2;
    Swap(a, b);                     // T = int
    cout << a << " " << b << endl;

    double d1 = 1.5, d2 = 2.5;
    Swap(d1, d2);                   // T = double，生成第二份代码
    cout << Max(d1, d2) << endl;    // Max<double>

    string s1 = "abc", s2 = "abd";
    Swap(s1, s2);
    cout << Max(s1, s2) << endl;    // Max<string>

    Date x{2024, 1, 1}, y{2024, 12, 31};
    cout << Max(x, y).m << endl;    // Max<Date>，要求 Date 支持 <
    return 0;
}
```

> 观察点：`Max(d1, d2)` 和 `Max(s1, s2)` 调的是**两个不同的函数**（`Max<double>`、`Max<string>`），这就是讲义说的「编译器根据实参推演生成对应类型的函数」。

---

## 练习 2　模板匹配规则实验　★★

**要求**：把讲义 P3～P5 的结论逐条**用代码验证**，每条打印一行标签，运行后对着输出看结论。

要验证的 5 条：

1. 非模板函数和同名函数模板可以共存；完全匹配时**优先调非模板**。
2. `Add<>(1, 2)` 用 `<>` 强制走模板。
3. `Add(1, 2.5)` 时模板 `T` 被推导成 `int` 又推导成 `double`，**模板推导失败**；最后是**普通函数**接住了它（隐式转换）——这就是「模板不做自动类型转换，普通函数做」。
4. 显式实例化 `Add<int>(1, 2.5)` 能编过，`2.5` 隐式转成 `2`。
5. 双模板参数 `Add2(1, 2.5)` 能推出 `T1=int, T2=double`，返回值按 `T1` 截断。

**参考代码**

```cpp
#include <iostream>
using namespace std;

// ① 非模板版本
int Add(int a, int b)
{
    cout << "[非模板]    ";
    return a + b;
}

// ② 函数模板版本
template<class T>
T Add(T a, T b)
{
    cout << "[模板T]     ";
    return a + b;
}

// ③ 双模板参数：T1 跟第一个实参，T2 跟第二个实参
template<class T1, class T2>
T1 Add2(T1 a, T2 b)
{
    cout << "[模板T1T2] ";
    return a + b;
}

int main()
{
    cout << Add(1, 2) << endl;        // ①②都能匹配 -> ① 非模板优先
    cout << Add<>(1, 2) << endl;      // 2 个 int，<> 强制走模板 -> ②
    cout << Add(1, 2.5) << endl;      // ② 推导失败(int vs double) -> ① 隐式转换
    cout << Add<int>(1, 2.5) << endl; // 显式实例化 T=int，2.5 -> 2 -> ②
    cout << Add2(1, 2.5) << endl;     // T1=int, T2=double -> ③，3.5 截断成 3
    return 0;
}
```

预期输出（左边是实际值）：

```
[非模板]    3
[模板T]     3
[非模板]    3
[模板T]     3
[模板T1T2] 3
```

**动手改一改**（这是理解「推导冲突」最关键的一步）：

1. 把 ① 注释掉，只留 ②③，`Add(1, 2.5)` 会报错：
   `no matching function for call to 'Add(int, double)'`，并提示
   `deduced conflicting types for parameter 'T' ('int' vs 'double')`。
2. 照讲义 P4 的两种修法各写一遍：

```cpp
int a = 10; double d = 20.5;

// 修法1：用户自己强转（模板就能推导了）
Add(a, (int)d);

// 修法2：显式实例化，编译器照着 <> 里的类型生成函数
Add<int>(a, d);
```

3. 注意最后一个 `Add2` 会得到 `C4244` 警告：`T2` 转 `T1` 可能丢数据。警告不是错，但它就是「隐式收窄」的提醒。

---

## 练习 3　泛型 Stack&lt;T&gt;（讲义 P5 的骨架补全）　★★★

**要求**

1. 按讲义把 `Stack<T>` 补完整：`Push` / `Pop` / `Top` / `Empty` / `Size`。
2. `Push` 要处理**扩容**（讲义里只写了注释「扩容」）：容量满了按 2 倍扩容，把旧数据搬过去再 `delete[]`。
3. 成员函数 `Push` **在类外定义**，体会 `template<class T> void Stack<T>::Push(...)` 的写法。
4. 用 `Stack<int>`、`Stack<double>`、`Stack<string>` 各测一遍。
5. 思考：如果允许 `Stack` 拷贝，会发生什么？（提示：和练习 13 的浅拷贝同一个坑）

**参考代码**

```cpp
#include <iostream>
#include <string>
#include <cassert>
using namespace std;

template<class T>
class Stack
{
public:
    Stack(size_t capacity = 4)
        : _array(new T[capacity])
        , _capacity(capacity)
        , _size(0)
    {}

    ~Stack() { delete[] _array; }

    // 现在先禁止拷贝，避免两个对象 delete 同一块内存（后面练深拷贝）
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    void Push(const T& data);          // 类内声明，类外定义

    void Pop()
    {
        assert(_size > 0);
        --_size;
    }

    T& Top()
    {
        assert(_size > 0);
        return _array[_size - 1];
    }

    const T& Top() const
    {
        assert(_size > 0);
        return _array[_size - 1];
    }

    bool Empty() const { return _size == 0; }
    size_t Size() const { return _size; }
    size_t Capacity() const { return _capacity; }

private:
    void CheckCapacity()
    {
        if (_size < _capacity) return;
        size_t newCap = _capacity == 0 ? 4 : _capacity * 2;
        T* tmp = new T[newCap];
        for (size_t i = 0; i < _size; ++i)
            tmp[i] = _array[i];        // 要求 T 可拷贝赋值
        delete[] _array;
        _array = tmp;
        _capacity = newCap;
    }

    T* _array;
    size_t _capacity;
    size_t _size;
};

// 类外定义成员函数：template 头 + Stack<T>::
template<class T>
void Stack<T>::Push(const T& data)
{
    CheckCapacity();
    _array[_size] = data;
    ++_size;
}

int main()
{
    Stack<int> st1;
    for (int i = 0; i < 10; ++i) st1.Push(i * i);
    cout << "size=" << st1.Size() << " cap=" << st1.Capacity() << endl;

    Stack<string> st2;
    st2.Push("hello");
    st2.Push("template");
    while (!st2.Empty())
    {
        cout << st2.Top() << " ";
        st2.Pop();
    }
    cout << endl;
    return 0;
}
```

> 关键理解：`Stack` 是**类名**，`Stack<int>` 才是**类型**（讲义 P6）。`Stack<int>` 和 `Stack<double>` 是两个毫不相干的类。

---

## 练习 4　支持范围 for 的 Array&lt;T&gt;　★★★

**要求**：写一个动态数组类模板 `Array<T>`，提供：

- `Array(size_t n)`、`size()`、`operator[]`（const 与非 const 两个版本）
- `begin()` / `end()`，返回 `T*`
- 然后写 `for (auto& e : arr) e *= 2;`，**验证范围 for 就是 begin/end 的语法糖**（讲义 P3）
- 最后用 `Array<int>`、`Array<double>`、`Array<string>` 测试

**参考代码**

```cpp
#include <iostream>
#include <string>
using namespace std;

template<class T>
class Array
{
public:
    explicit Array(size_t n = 0) : _a(new T[n]()), _n(n) {}
    ~Array() { delete[] _a; }

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;

    size_t size() const { return _n; }

    T& operator[](size_t i) { return _a[i]; }
    const T& operator[](size_t i) const { return _a[i]; }

    T* begin() { return _a; }
    T* end() { return _a + _n; }
    const T* begin() const { return _a; }
    const T* end() const { return _a + _n; }

private:
    T* _a;
    size_t _n;
};

int main()
{
    Array<int> a(5);
    for (size_t i = 0; i < a.size(); ++i) a[i] = (int)i + 1;

    for (auto& e : a) e *= 2;             // 能改原数组
    for (auto e : a) cout << e << " ";    // 拷贝，不改原数组
    cout << endl;

    Array<string> s(3);
    s[0] = "template"; s[1] = "is"; s[2] = "fun";
    for (const auto& str : s) cout << str << " ";
    cout << endl;

    return 0;
}
```

> 把 `begin()/end()` 删掉再编译，范围 for 立刻报错——这就证明了它底层调的就是这两个函数。

---

# 第二部分：STL 简介（07）

## 练习 5　六大组件各用一次　★★

**要求**：写一个程序，把 STL 六大组件**各摸一遍**（不用深入，知道谁是谁就行）：

| 组件 | 本练习里的代表 |
|---|---|
| 容器 | `vector`、`stack` |
| 迭代器 | `v.begin()` / `v.end()`、反向迭代器 |
| 算法 | `sort`、`reverse`、`max_element` |
| 仿函数 | `greater<int>` |
| 适配器 | `stack`（容器适配器） |
| 空间配置器 | `vector<int>` 背后的 `allocator<int>`（了解即可） |

任务：输入一组整数存进 `vector`，降序排序并遍历输出，再用 `stack` 逆序输出，最后用迭代器找出最大值。

**参考代码**

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <stack>
using namespace std;

int main()
{
    vector<int> v = {5, 1, 4, 2, 3};              // 容器

    // 算法 + 迭代器 + 仿函数
    sort(v.begin(), v.end(), greater<int>());

    // 迭代器遍历（C++98 风格）
    for (vector<int>::iterator it = v.begin(); it != v.end(); ++it)
        cout << *it << " ";
    cout << endl;

    // 容器适配器
    stack<int> st;
    for (int x : v) st.push(x);
    while (!st.empty())
    {
        cout << st.top() << " ";
        st.pop();
    }
    cout << endl;

    // 算法 + 迭代器：找最大值，返回迭代器
    vector<int>::iterator itMax = max_element(v.begin(), v.end());
    cout << "max = " << *itMax << endl;

    // 反向迭代器
    for (auto rit = v.rbegin(); rit != v.rend(); ++rit)
        cout << *rit << " ";
    cout << endl;

    return 0;
}
```

> 讲义 P3 说的「能用」就是这个层次：知道什么场景该拿哪个组件出来用。

---

## 练习 6　sort 的自定义比较 + vector / list 对比　★★

**要求**

1. 有一个 `vector<string>`，按**长度升序**排序，长度相同按**字典序**。分别用**仿函数**和 **lambda** 写比较器。
2. 同样的数据放进 `list<string>`，用 `lt.sort(...)` 排序，**不要**用 `std::sort(lt.begin(), lt.end())`，观察报错并解释原因。
3. 思考：为什么 `vector` 能用 `std::sort`，`list` 不能？

**参考代码**

```cpp
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
using namespace std;

// 仿函数
struct CmpByLen
{
    bool operator()(const string& a, const string& b) const
    {
        if (a.size() != b.size()) return a.size() < b.size();
        return a < b;
    }
};

int main()
{
    vector<string> v = {"banana", "kiwi", "apple", "fig", "pear"};

    sort(v.begin(), v.end(), CmpByLen());          // 仿函数
    for (const auto& s : v) cout << s << " ";
    cout << endl;

    sort(v.begin(), v.end(), [](const string& a, const string& b) {  // lambda
        return a.size() != b.size() ? a.size() < b.size() : a < b;
    });
    for (const auto& s : v) cout << s << " ";
    cout << endl;

    list<string> lt = {"banana", "kiwi", "apple", "fig", "pear"};
    lt.sort([](const string& a, const string& b) {
        return a.size() != b.size() ? a.size() < b.size() : a < b;
    });
    for (const auto& s : lt) cout << s << " ";
    cout << endl;

    // 下面这行会编译失败：list 的迭代器是双向迭代器，不满足随机访问
    // sort(lt.begin(), lt.end());
    return 0;
}
```

> 报错关键词：`no match for 'operator-'`。因为 `std::sort` 内部要做 `it + n`、`it1 - it2`，而 `list` 迭代器只能 `++`/`--`。这就是讲义 P3「明理」的第一课：**算法对迭代器有要求**。

---

# 第三部分：string（08）

## 练习 7　仅仅反转字母（LeetCode 917）　★★

**要求**：给定字符串 `s`，只反转其中的字母，非字母留在原位。

```
输入: "a-bC-dEf-ghIj"
输出: "j-Ih-gfE-dCba"
```

**要练的点**：`operator[]`、`size()`、双指针、`swap`、`isalpha`。

**参考代码**

```cpp
#include <string>
#include <cctype>
using namespace std;

class Solution
{
public:
    string reverseOnlyLetters(string s)
    {
        int begin = 0, end = (int)s.size() - 1;
        while (begin < end)
        {
            while (begin < end && !isalpha((unsigned char)s[begin])) ++begin;
            while (begin < end && !isalpha((unsigned char)s[end]))   --end;
            swap(s[begin], s[end]);
            ++begin;
            --end;
        }
        return s;
    }
};
```

> 对比讲义 P7 的写法：讲义用 `ch >= 'a' && ch <= 'z'` 手写判断，这里用 `isalpha`。
> 注意参数要转 `unsigned char`，否则遇到中文/负数 `char` 是未定义行为。

---

## 练习 8　第一个只出现一次的字符（LeetCode 387 / 剑指 Offer 50）　★★

**要求**：返回第一个只出现一次的字符的**下标**，不存在返回 `-1`。

```
输入: "loveleetcode"   输出: 2（字符 'v'）
```

**要练的点**：`operator[]` 遍历、计数数组、字符作下标。

**参考代码**

```cpp
#include <string>
using namespace std;

class Solution
{
public:
    int firstUniqChar(string s)
    {
        int count[256] = {0};
        for (unsigned char ch : s)
            ++count[ch];

        for (int i = 0; i < (int)s.size(); ++i)
            if (count[(unsigned char)s[i]] == 1)
                return i;

        return -1;
    }
};
```

> **讲义代码有个小坑**：P7 写的是 `count[s[i]]`。如果 `char` 是 `signed char`，遇到非 ASCII 字符会得到负下标，越界。
> 自己动手把 `(unsigned char)` 去掉，喂一个中文字符串试试，看会不会崩。

---

## 练习 9　最后一个单词的长度（LeetCode 58 / 华为 HJ1）　★★

**要求**：给一行字符串（含空格），输出最后一个单词的长度。**必须用 `getline` 读**，不能用 `cin >>`。

```
输入: "hello nowcoder"
输出: 8
```

**要练的点**：`getline`（讲义 P5）、`rfind`、`size()`、尾空格处理。

**参考代码**

```cpp
#include <iostream>
#include <string>
using namespace std;

int main()
{
    string line;
    while (getline(cin, line))          // 不能用 cin >> line，遇空格就停
    {
        int i = (int)line.size() - 1;

        while (i >= 0 && line[i] == ' ') --i;   // 跳过末尾空格
        int end = i;

        while (i >= 0 && line[i] != ' ') --i;   // 找到最后一个单词的起点

        cout << end - i << endl;
    }
    return 0;
}
```

**进阶**：再用 `rfind(' ')` 写一版（讲义 P8 的写法）：

```cpp
size_t pos = line.rfind(' ');
cout << line.size() - pos - 1 << endl;      // pos 为 npos 时也能算对
```

> 想一想：`line` 为空串时，`line.size() - 1` 是多少？（`size_t` 下溢成巨大值，这是经典坑。）

---

## 练习 10　验证回文串（LeetCode 125）　★★

**要求**：只考虑字母和数字，忽略大小写，判断是否回文。空串算回文。

```
输入: "A man, a plan, a canal: Panama"   输出: true
输入: "race a car"                       输出: false
```

**要练的点**：`operator[]`、双指针、`tolower`、范围 for 修改字符（讲义 P3）。

**参考代码**

```cpp
#include <string>
#include <cctype>
using namespace std;

class Solution
{
public:
    bool isPalindrome(string s)
    {
        int begin = 0, end = (int)s.size() - 1;

        while (begin < end)
        {
            while (begin < end && !isalnum((unsigned char)s[begin])) ++begin;
            while (begin < end && !isalnum((unsigned char)s[end]))   --end;

            if (tolower((unsigned char)s[begin]) != tolower((unsigned char)s[end]))
                return false;

            ++begin;
            --end;
        }
        return true;
    }
};
```

**加练**：讲义 P8 的写法是「先整体转大写再判断」。用范围 for 实现：

```cpp
for (auto& ch : s)
    if (ch >= 'a' && ch <= 'z')
        ch -= 32;
```

> 注意 `auto&`，写 `auto ch` 改的是副本，原串不变——这是范围 for 最常见的错误。

---

## 练习 11　字符串相加（LeetCode 415）　★★★

**要求**：两个非负整数字符串，返回它们的和（不能直接转 `int`，会溢出）。

```
输入: "123", "456"     输出: "579"
输入: "1", "9999"      输出: "10000"
```

**要练的点**：`operator[]`、`+=` 尾插、`reverse`、进位处理。

**参考代码**

```cpp
#include <string>
#include <algorithm>
using namespace std;

class Solution
{
public:
    string addStrings(string num1, string num2)
    {
        int i = (int)num1.size() - 1;
        int j = (int)num2.size() - 1;
        int carry = 0;
        string res;

        while (i >= 0 || j >= 0 || carry)
        {
            int a = i >= 0 ? num1[i--] - '0' : 0;
            int b = j >= 0 ? num2[j--] - '0' : 0;

            int sum = a + b + carry;
            res += char('0' + sum % 10);    // 先尾插低位
            carry = sum / 10;
        }

        reverse(res.begin(), res.end());    // 再整体反转
        return res;
    }
};
```

**变体练习**（讲义 P9 提的另一种做法）：用 `insert(begin(), ...)` 头插，比较两种写法的效率差异。头插是 O(n²)，尾插 + reverse 是 O(n)。

---

## 练习 12　翻转字符串 II / III（讲义课后作业）　★★★

### 12.1 反转字符串 II（LeetCode 541）

每 `2k` 个字符，翻转前 `k` 个；不足 `k` 个则全翻。

```
输入: s = "abcdefg", k = 2
输出: "bacdfeg"
```

```cpp
#include <string>
#include <algorithm>
using namespace std;

class Solution
{
public:
    string reverseStr(string s, int k)
    {
        size_t step = 2 * (size_t)k;
        for (size_t i = 0; i < s.size(); i += step)
        {
            size_t end = min(i + (size_t)k, s.size());
            reverse(s.begin() + i, s.begin() + end);
        }
        return s;
    }
};
```

### 12.2 反转字符串中的单词 III（LeetCode 557）

单词顺序不变，单词内部反转。

```
输入: "Let's take LeetCode contest"
输出: "s'teL ekat edoCteeL tsetnoc"
```

```cpp
#include <string>
#include <algorithm>
using namespace std;

class Solution
{
public:
    string reverseWords(string s)
    {
        size_t i = 0, n = s.size();
        while (i < n)
        {
            size_t j = i;
            while (j < n && s[j] != ' ') ++j;   // 定位一个单词 [i, j)
            reverse(s.begin() + i, s.begin() + j);
            i = j + 1;                          // 跳过空格
        }
        return s;
    }
};
```

> 讲义 P10 把「翻转字符串 II / III」「字符串相乘」都列成了课后作业，这两题+第 11 题基本覆盖了 string 的遍历类操作。

---

## 练习 13　模拟实现 MyString（讲义 P10～P13）　★★★★

这是三讲里**最重要**的一题，面试常考，也是理解「浅拷贝/深拷贝」的唯一途径。

**要求**

1. 先写**传统版**：构造、拷贝构造、`operator=`、析构，全部深拷贝。
2. 再写**现代版**：拷贝构造用「构造临时对象 + swap」，赋值用「传值 + swap」。
3. 补齐接口：`operator[]`、`size`、`capacity`、`c_str`、`empty`、`reserve`、`push_back`、`append`、`operator+=`、`operator<<`。
4. 写测试：拷贝、赋值、**自赋值** `s = s`、链式赋值 `a = b = c`、按值传参。
5. **故意制造浅拷贝崩溃**：把拷贝构造和赋值注释掉，运行 `MyString a("hi"); MyString b(a);`，看重复 `delete[]` 崩溃。

**参考代码（现代版）**

```cpp
#include <iostream>
#include <cstring>
#include <cassert>
#include <algorithm>
using namespace std;

class MyString
{
public:
    MyString(const char* str = "")
        : _size(strlen(str)), _capacity(_size), _str(new char[_size + 1])
    {
        assert(str != nullptr);
        strcpy(_str, str);
    }

    MyString(const MyString& s)
        : _size(s._size), _capacity(s._capacity), _str(new char[_capacity + 1])
    {
        strcpy(_str, s._str);
    }

    // 传值：实参已经是拷贝，直接交换即可；自赋值也安全
    MyString& operator=(MyString s)
    {
        Swap(s);
        return *this;
    }

    ~MyString() { delete[] _str; }

    void Swap(MyString& s)
    {
        std::swap(_str, s._str);
        std::swap(_size, s._size);
        std::swap(_capacity, s._capacity);
    }

    const char* c_str() const { return _str; }
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    bool empty() const { return _size == 0; }

    char& operator[](size_t i) { assert(i <= _size); return _str[i]; }
    const char& operator[](size_t i) const { assert(i <= _size); return _str[i]; }

    void reserve(size_t n)
    {
        if (n <= _capacity) return;
        char* tmp = new char[n + 1];
        strcpy(tmp, _str);
        delete[] _str;
        _str = tmp;
        _capacity = n;
    }

    void push_back(char ch)
    {
        if (_size == _capacity)
            reserve(_capacity == 0 ? 4 : _capacity * 2);
        _str[_size++] = ch;
        _str[_size] = '\0';
    }

    void append(const char* s)
    {
        size_t len = strlen(s);
        reserve(_size + len);
        strcpy(_str + _size, s);
        _size += len;
    }

    MyString& operator+=(char ch)      { push_back(ch); return *this; }
    MyString& operator+=(const char* s){ append(s);    return *this; }

private:
    // 注意：成员按「声明顺序」初始化，_size 必须写在 _str 前面，
    // 否则 _str(new char[_size + 1]) 用的是还没初始化的 _size
    size_t _size;
    size_t _capacity;
    char* _str;
};

ostream& operator<<(ostream& out, const MyString& s)
{
    out << s.c_str();
    return out;
}

int main()
{
    MyString s1("hello");
    MyString s2(s1);              // 拷贝构造：深拷贝
    MyString s3("world");
    s3 = s2;                      // 赋值
    s3 = s3;                      // 自赋值，不能崩
    MyString s4;
    s4 = s1;                      // 空串 -> 非空
    s4 += ' ';
    s4 += "template";

    cout << s1 << " | " << s2 << " | " << s3 << " | " << s4 << endl;
    cout << "size=" << s4.size() << " cap=" << s4.capacity() << endl;

    // 链式赋值
    MyString a, b, c;
    a = b = c = s1;
    cout << a << b << c << endl;

    // 按值传参：会调用拷贝构造
    auto ByValue = [](MyString s) { return s.size(); };
    cout << ByValue(s1) << endl;

    return 0;
}
```

**传统版对照**（讲义 P12，自己敲一遍）：

```cpp
MyString(const MyString& s)
    : _str(new char[strlen(s._str) + 1])
{
    strcpy(_str, s._str);
}

MyString& operator=(const MyString& s)
{
    if (this != &s)                       // 必须判自赋值
    {
        char* tmp = new char[strlen(s._str) + 1];
        strcpy(tmp, s._str);
        delete[] _str;                    // 先申请再释放，异常安全
        _str = tmp;
    }
    return *this;
}
```

**浅拷贝实验**

```cpp
class BadString
{
public:
    BadString(const char* s = "") : _str(new char[strlen(s) + 1]) { strcpy(_str, s); }
    ~BadString() { delete[] _str; }       // 不写拷贝构造 = 浅拷贝
private:
    char* _str;
};

int main()
{
    BadString a("hello");
    BadString b(a);       // 两个指针指向同一块内存
    return 0;             // 析构两次 -> double free，进程崩溃
}
```

> 用 VS 的 Debug 模式或 `-fsanitize=address` 跑，报告会直接指出 `double-free`。
> 这就是讲义 P11「像父母只买了一份玩具，两个孩子抢」的现场版。

**思考题**（对应讲义 P14 写时拷贝）

1. 现代版 `operator=(MyString s)` 和传统版 `operator=(const MyString&)` 各自优劣势？
2. 为什么现代版传值 + swap 天然支持自赋值？
3. 写时拷贝（引用计数）能省掉哪次拷贝？它读操作的缺陷是什么？

---

## 练习 14　auto 与范围 for 改写练习　★

**要求**：把一段 C++98 风格的遍历代码，用 `auto` + 范围 for 重写；再故意触发讲义里的 3 个 `auto` 报错，把报错信息抄下来。

```cpp
#include <iostream>
#include <string>
#include <map>
#include <vector>
using namespace std;

int main()
{
    map<string, string> dict = {
        {"apple", "苹果"}, {"orange", "橙子"}, {"pear", "梨"}
    };

    // C++98 写法，用 auto 改写
    for (auto it = dict.begin(); it != dict.end(); ++it)
        cout << it->first << ":" << it->second << endl;

    vector<string> words = {"hello", "template", "string"};

    // 范围 for + auto&：真正修改容器元素
    for (auto& w : words)
        for (auto& ch : w)
            ch = (char)toupper((unsigned char)ch);

    for (const auto& w : words) cout << w << " ";
    cout << endl;

    int arr[] = {1, 2, 3};
    for (auto& e : arr) e *= 2;          // 改原数组
    for (auto e : arr) cout << e << " ";

    return 0;
}
```

**三个报错实验**（讲义 P1～P2，逐个放开注释看报错）：

```cpp
auto e;                    // error C3531: 类型包含 auto 的符号必须具有初始值
void f(auto a) {}          // error C3533: 参数不能包含 auto
auto cc = 3, dd = 4.0;     // error C3538: 声明符列表中 auto 必须始终推导为同一类型
auto arr2[] = {1, 2, 3};   // error C3318: 数组不能具有包含 auto 的元素类型
```

**思考**：`auto` 声明引用时必须写 `&`（`auto& m = x;`），为什么？

---

# 选做 / 进阶

1. **字符串相乘（LeetCode 43）**：两个大数字符串相乘，用「竖式 + 结果数组」实现，长度 `m+n`。
2. **给 `MyString` 加 `operator+`、`find`、`substr`**：对照 `std::string` 的接口表（讲义 P5）逐个实现。
3. **把 `Stack<T>` 的拷贝改成深拷贝**，然后 `Stack<int> a; a.Push(1); Stack<int> b(a);` 不再崩。
4. **模板 + STL 结合**：写 `template<class T> T Sum(const vector<T>& v)`，对 `int`、`double`、`string`（拼接）都能用。
5. **给 `Array<T>` 加 `push_back`**，体会模板类里「扩容」和 `Stack<T>` 里是同一套逻辑。

---

# 自测清单（能全部打勾就可以进下一讲）

- [ ] 能默写函数模板格式，并说出 `typename` 和 `class` 的区别（讲义 P3）
- [ ] 能解释 `Add(1, 2.5)` 为什么报错，并给出 2 种修法
- [ ] 能说出「非模板优先」和「更匹配的模板优先」分别什么时候发生
- [ ] 能默写 `template<class T> void Stack<T>::Push(...)` 这种类外定义
- [ ] 能说出 `Stack` 和 `Stack<int>` 谁是类、谁是类型
- [ ] 能说出 STL 六大组件，并各举一个例子
- [ ] 能解释 `std::sort` 为什么不能用在 `list` 上
- [ ] 能默写 `size/length/capacity/reserve/resize/clear` 的区别
- [ ] 能说出 `getline` 和 `cin >>` 的区别
- [ ] 能手写 `MyString` 的深拷贝构造和赋值（传统版 + 现代版）
- [ ] 能解释浅拷贝为什么会 double free
- [ ] 能说出范围 for 的底层是 `begin()/end()`
