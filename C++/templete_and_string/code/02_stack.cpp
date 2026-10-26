// 练习 3：类模板 Stack<T>
// 对应讲义 06.模板初阶 P5-P6
// 重点：模板参数在类外定义成员函数、容量扩容
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

    // 先禁止拷贝，避免两个对象 delete 同一块内存
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
            tmp[i] = _array[i];
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
    for (int i = 0; i < 10; ++i)
        st1.Push(i * i);
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
