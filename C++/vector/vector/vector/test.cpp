#define _CRT_SECURE_NO_WARNINGS 1 
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
#include<vector>
using namespace std;

void Print(const vector<int>& v)
{
	for (size_t i = 0; i < v.size(); i++)
	{
		cout << v[i] << " ";
	}
	cout << endl;

	/*for (auto e : v)
	{
		cout << e << " ";
	}
	cout << endl;

	vector<int>::const_iterator it = v.begin();
	while (it != v.end())
	{
		cout << *it << " ";
		++it;
	}
	cout << endl;*/
}

void test_vector1()
{
	vector<int> v1;
	vector<int> v2(10, 1);
	vector<int> v3(v2.begin(), v2.end());
	string s1("xxxxxxxxxxxxx");
	vector<int> v4(s1.begin(), s1.end());
	vector<int> v5(v3);

	//vector<int> v6({ 1,2,3,4,5 });
	vector<int> v6 = { 1,2,3,4,5 };
	vector<int> v7 = { 1,2,3,4,5,1,1,1,1,1,1 };

	Print(v2);
	Print(v4);
	Print(v6);
	Print(v7);


	auto il = { 10, 20, 30,1,2,2 };
	for (auto e : il)
	{
		cout << e << " ";
	}
	cout << endl;
}

void test_vector2()
{
	vector<int> v1;
	//const int n = 10000000;
	const int n = 100;
	//v1.reserve(n);

	size_t old = v1.capacity();
	//cout << v1.capacity() << endl;
	size_t begin = clock();
	for (size_t i = 0; i < n; i++)
	{
		v1.push_back(i);
		/*
		if (old != v1.capacity())
		{
			cout << v1.capacity() << endl;
			old = v1.capacity();
		}
		*/
	}
	size_t end = clock();
	cout << end - begin << endl;

	vector<int> v2;
	v2.resize(100, 1);
	Print(v2);
}

void test_vector3()
{
	vector<int> v1 = { 1,2,3,4,5 };
	v1.push_back(6);
	Print(v1);

	// 头插
	v1.insert(v1.begin(), 0);
	Print(v1);

	v1.insert(v1.begin() + 3, 0);
	Print(v1);

	// 头删
	v1.erase(v1.begin());
	Print(v1);

	v1.erase(v1.begin() + 3);
	Print(v1);
}

struct AA
{
	int _a1 = 1;
	int _a2 = 1;

	AA(int a1 = 1, int a2 = 1)
		:_a1(a1)
		, _a2(a2)
	{
	}
};

//template<class T>
//class vector
//{
//private:
//	T*	   _a;
//	size_t _size;
//	size_t _capacity;
//};

void test_vector4()
{
	AA aa1 = { 0, 0 };
	vector<AA> v = { aa1, {1,1}, {2,2}, {3,3} };
	auto it = v.begin();
	while (it != v.end())
	{
		cout << it->_a1 << ":" << it->_a2 << endl;
		++it;
	}
	cout << endl;

	v.push_back(aa1);
	v.emplace_back(aa1);

	// 差异, 推荐
	v.emplace_back(1, 1);
	v.push_back({ 2,2 });

	it = v.begin();
	while (it != v.end())
	{
		cout << it->_a1 << ":" << it->_a2 << endl;
		++it;
	}
	cout << endl;
}
//
//int main()
//{
//	test_vector4();
//
//	return 0;
//}

////////////////////////////////////////////////////////////////////////
#include"vector.h"

namespace bit
{
	void Print(const vector<int>& v)
	{
		for (auto e : v)
		{
			cout << e << " ";
		}
		cout << endl;

		//for (size_t i = 0; i < v.size(); i++)
		//{
		//	// v[0]++;
		//	cout << v[i] << " ";
		//}
		//cout << endl;
	}

	void test_vector1()
	{
		bit::vector<int> v;
		v.push_back(1);
		v.push_back(2);
		v.push_back(3);
		v.push_back(4);
		v.push_back(5);
		v.push_back(5);
		v.push_back(5);
		v.push_back(5);
		v.push_back(5);
		v.push_back(5);

		v[0]++;

		Print(v);
		for (auto e : v)
		{
			cout << e << " ";
		}
		cout << endl;
	}

	void test_vector2()
	{
		bit::vector<int> v;
		v.push_back(1);
		v.push_back(2);
		v.push_back(3);
		v.push_back(4);
		//v.push_back(5);
		Print(v);

		v.insert(v.begin(), 0);
		Print(v);

		auto it = v.begin() + 3;
		// insert以后，it是否失效？
		// it失效了，也就意味着，insert以后，it失效了，it就不能使用了
		v.insert(it, 30);
		Print(v);
	}

	void test_vector3()
	{
		bit::vector<int> v;
		v.push_back(1);
		v.push_back(2);
		v.push_back(3);
		v.push_back(4);
		v.push_back(5);
		Print(v);

		v.erase(v.begin());
		Print(v);

		auto it = v.begin() + 2;
		// it是否失效呢？失效，不能访问，访问结果未定义
		v.erase(it);
		Print(v);
	}

	void test_vector4()
	{
		bit::vector<int> v;
		v.push_back(1);
		v.push_back(2);
		v.push_back(2);
		v.push_back(3);
		v.push_back(4);
		v.push_back(5);
		v.push_back(6);
		// Print(v);

		for (auto e : v)
		{
			cout << e << " ";
		}
		cout << endl;

		// 删除所有的偶数
		/*auto it = v.begin();
		while (it != v.end())
		{
			if (*it % 2 == 0)
			{
				v.erase(it);
			}

			++it;
		}*/

		auto it = v.begin();
		while (it != v.end())
		{
			if (*it % 2 == 0)
			{
				it = v.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto e : v)
		{
			cout << e << " ";
		}
		cout << endl;
	}

	void test_vector5()
	{
		bit::vector<int> v;
		v.push_back(1);
		v.push_back(2);
		v.push_back(2);
		v.push_back(3);
		v.push_back(4);
		v.push_back(5);
		v.push_back(6);
		for (auto e : v)
		{
			cout << e << " ";
		}
		cout << endl;

		v.resize(3);
		for (auto e : v)
		{
			cout << e << " ";
		}
		cout << endl;

		// v.resize(20);
		v.resize(20, 5);
		for (auto e : v)
		{
			cout << e << " ";
		}
		cout << endl;
	}

	void test_vector6()
	{
		bit::vector<int> v1;
		v1.push_back(1);
		v1.push_back(2);
		v1.push_back(2);
		v1.push_back(3);
		v1.push_back(4);
		v1.push_back(5);
		v1.push_back(6);
		for (auto e : v1)
		{
			cout << e << " ";
		}
		cout << endl;

		bit::vector<int> v2(v1);

		for (auto e : v2)
		{
			cout << e << " ";
		}
		cout << endl;

		// bit::vector<int> v3({ 10,20,30,40 });
		bit::vector<int> v3 = { 10,20,30,40 };
		v1 = v3;
		for (auto e : v1)
		{
			cout << e << " ";
		}
		cout << endl;

		//bit::vector<int> v4(10u, 1);
		bit::vector<int> v4(10, 1);
		for (auto e : v4)
		{
			cout << e << " ";
		}
		cout << endl;

		bit::vector<char> v5(10, 'x');
	}

	void test_vector7()
	{
		bit::vector<string> v1;
		v1.push_back("111111111111111111111111");
		v1.push_back("111111111111111111111111");
		v1.push_back("111111111111111111111111");
		v1.push_back("111111111111111111111111");
		v1.push_back("111111111111111111111111");
		for (auto& e : v1)
		{
			cout << e << " ";
		}
		cout << endl;
	}
}

//int main()
//{
//	bit::test_vector7();
//
//	int i = 0;
//	int j = int();
//	int k = int(1);
//	// C++11
//	int y = {};
//	int t = {1};
//	int z{ 2 };
//	int m{};
//
//	return 0;
//}

#include<list>
#include<algorithm>

int main()
{
	list<int> lt1;
	list<int> lt2 = { 1,2,3,4,5 };
	list<int>::iterator it2 = lt2.begin();
	while (it2 != lt2.end())
	{
		cout << *it2 << " ";
		++it2;
	}
	cout << endl;

	for (auto e : lt2)
	{
		cout << e << " ";
	}
	cout << endl;

	auto pos = find(lt2.begin(), lt2.end(), 3);
	if (pos != lt2.end())
	{
		lt2.insert(pos, 30); // pos没有失效，因为没有扩容
		lt2.erase(pos);	     // pos失效了
		// cout << *pos << endl;
	}

	for (auto e : lt2)
	{
		cout << e << " ";
	}
	cout << endl;

	bit::vector<int> v3 = { 10,20,30,40 };

	//sort(lt2.begin(), lt2.end()); // 不支持
	lt2.sort();
	sort(v3.begin(), v3.end());

	list<int> lt3 = { 1,2,2,3,3,2,3,4,5 };
	for (auto e : lt3)
	{
		cout << e << " ";
	}
	cout << endl;
	lt3.sort();
	lt3.unique(); // 去重
	for (auto e : lt3)
	{
		cout << e << " ";
	}
	cout << endl;

	// 4这个节点挪到头位置
	list<int> lt4 = { 1,2,3,4,5 };
	for (auto e : lt4)
	{
		cout << e << " ";
	}
	cout << endl;
	pos = find(lt4.begin(), lt4.end(), 4);
	lt4.splice(lt4.begin(), lt4, pos);
	for (auto e : lt4)
	{
		cout << e << " ";
	}
	cout << endl;


	return 0;
}