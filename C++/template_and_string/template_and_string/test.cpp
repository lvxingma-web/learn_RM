#define _CRT_SECURE_NO_WARNINGS 1 
#include<iostream>
#include<string>

//函数模板
template<class T>//关键词template
T add(const T& a,const T& b) {
	return a + b;
}

//类模板
//类模板成员函数生命与定义分离需要指定类域以及显示模板
//void stack::push_back<T>(const T& x)
template<class T>
struct ListNode
{
	T _data;
	ListNode<T>* _next;

	ListNode(ListNode<T>* next, const T& data)
		: _next(next)
		, _data(data)
	{
	}
};

void test01() {

	//int a = 1, b = 2;
	int a = 1, b = 2.5;
	auto c = add(a, (int)b);//隐式调用(可能调用现成的，不会调用模板)
	auto d = add<int>(a, b);//显示调用
	std::cout << c << std::endl;
}

// 1.Swap,Max
template<class T>
void swap(T& a, T& b) {
	T c = a;
	a = b;
	b = c;
}

template<class T>
T& Max(const T& a, const T& b) {
	return a > b ? a : b;
}

void test2() {
	std::string s1 = "abc", s2 = "bcd";
	swap(s1, s2);
	std::cout << s1 << ' ' << s2 << std::endl;

}

//new delete
void test03() {
	int* a = new int[2] { 1, 3 };
	auto b = new int[2] {1, 3};

	//std::cout << a[1] << "\n" << b[1] << std::endl;
	delete[] a;
	delete[] b;
}

int main() {
	//test01();
	//test2();
	test03();
	return 0;
}