// 练习 13：模拟实现 MyString（深拷贝，现代版）
// 对应讲义 08.string P10-P14
// 重点：拷贝构造、赋值、析构必须显式给出；传值 + swap 的现代写法
#include <iostream>
#include <cstring>
#include <cassert>
#include <utility>
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

    MyString& operator+=(char ch) { push_back(ch); return *this; }
    MyString& operator+=(const char* s) { append(s); return *this; }

private:
    // 成员按「声明顺序」初始化，_size 必须在 _str 前面
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

    return 0;
}
