// 练习 4：类模板 Array<T>，begin/end 与范围 for
// 对应讲义 06.模板初阶 + 08.string 的 auto / 范围 for
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
    for (size_t i = 0; i < a.size(); ++i)
        a[i] = (int)i + 1;

    for (auto& e : a) e *= 2;             // 能改原数组
    for (auto e : a) cout << e << " ";    // 拷贝，不改原数组
    cout << endl;

    Array<string> s(3);
    s[0] = "template";
    s[1] = "is";
    s[2] = "fun";
    for (const auto& str : s)
        cout << str << " ";
    cout << endl;

    return 0;
}
