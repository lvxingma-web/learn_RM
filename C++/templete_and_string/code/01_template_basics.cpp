// 练习 1 + 练习 2：函数模板、重载匹配规则
// 对应讲义 06.模板初阶 P2-P5
#include <iostream>
#include <string>
using namespace std;

// ---------- 练习 1：通用 Swap / Max ----------
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

// ---------- 练习 2：模板匹配规则 ----------
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
    cout << "===== 练习 1: Swap / Max =====" << endl;
    int a = 1, b = 2;
    Swap(a, b);                      // T = int
    cout << a << " " << b << endl;

    double d1 = 1.5, d2 = 2.5;
    Swap(d1, d2);                    // T = double，生成第二份代码
    cout << Max(d1, d2) << endl;

    string s1 = "abc", s2 = "abd";
    Swap(s1, s2);
    cout << Max(s1, s2) << endl;

    cout << "\n===== 练习 2: 重载匹配 =====" << endl;
    cout << Add(1, 2) << endl;        // ①②都能匹配 -> ① 非模板优先
    cout << Add<>(1, 2) << endl;      // <> 强制走模板 -> ②
    cout << Add(1, 2.5) << endl;      // ② 推导失败(int vs double) -> ①
    cout << Add<int>(1, 2.5) << endl; // 显式实例化 T=int，2.5 -> 2 -> ②
    cout << Add2(1, 2.5) << endl;     // T1=int, T2=double -> ③

    return 0;
}
