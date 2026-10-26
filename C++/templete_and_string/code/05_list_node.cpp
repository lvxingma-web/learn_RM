// 链表节点模板 + 带头单链表
// 对应讲义 06.模板初阶（类模板）
// 重点：节点析构不删 _next；链表负责 new/delete；拷贝要深拷贝
#include <iostream>
#include <string>
#include <utility>
using namespace std;

// ---------------- 节点 ----------------
template<class T>
struct ListNode
{
    T _data;
    ListNode<T>* _next;

    ListNode(ListNode<T>* next, const T& data)
        : _next(next)
        , _data(data)
    {}
};

// ---------------- 链表 ----------------
template<class T>
class List
{
    typedef ListNode<T> Node;

public:
    List() : _head(nullptr), _tail(nullptr), _size(0) {}

    // 深拷贝
    List(const List& lt) : _head(nullptr), _tail(nullptr), _size(0)
    {
        for (Node* cur = lt._head; cur; cur = cur->_next)
            PushBack(cur->_data);
    }

    // 传值 + swap：拷贝赋值和自赋值都安全
    List& operator=(List lt)
    {
        Swap(lt);
        return *this;
    }

    ~List() { Clear(); }

    void Swap(List& lt)
    {
        std::swap(_head, lt._head);
        std::swap(_tail, lt._tail);
        std::swap(_size, lt._size);
    }

    void PushBack(const T& x)
    {
        Node* newnode = new Node(nullptr, x);
        if (_tail == nullptr)
        {
            _head = _tail = newnode;
        }
        else
        {
            _tail->_next = newnode;
            _tail = newnode;
        }
        ++_size;
    }

    void PushFront(const T& x)
    {
        Node* newnode = new Node(_head, x);
        _head = newnode;
        if (_tail == nullptr) _tail = newnode;
        ++_size;
    }

    void PopFront()
    {
        if (_head == nullptr) return;
        Node* next = _head->_next;
        delete _head;
        _head = next;
        if (_head == nullptr) _tail = nullptr;
        --_size;
    }

    void Clear()
    {
        Node* cur = _head;
        while (cur)
        {
            Node* next = cur->_next;
            delete cur;
            cur = next;
        }
        _head = _tail = nullptr;
        _size = 0;
    }

    size_t Size() const { return _size; }
    bool Empty() const { return _head == nullptr; }
    T& Front() { return _head->_data; }

    void Print() const
    {
        for (Node* cur = _head; cur; cur = cur->_next)
            cout << cur->_data << " ";
        cout << endl;
    }

private:
    Node* _head;
    Node* _tail;
    size_t _size;
};

int main()
{
    List<int> lt;
    lt.PushBack(1);
    lt.PushBack(2);
    lt.PushBack(3);
    lt.PushFront(0);
    lt.Print();

    List<int> lt2(lt);          // 拷贝构造
    lt2.PushBack(4);
    lt.Print();
    lt2.Print();

    List<int> lt3;
    lt3 = lt2;                  // 拷贝赋值
    lt3.PopFront();
    lt3.Print();

    lt3 = lt3;                  // 自赋值
    lt3.Print();

    List<string> ls;
    ls.PushBack("hello");
    ls.PushBack("list");
    ls.Print();

    cout << "size=" << lt.Size() << " front=" << lt.Front() << endl;
    return 0;
}
