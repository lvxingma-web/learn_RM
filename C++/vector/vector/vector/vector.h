#pragma once

#include<assert.h>

namespace bit
{
	template<class T>
	class vector
	{
	public:
		// typedef T* iterator;
		using iterator = T*;
		using const_iterator = const T*;

		iterator begin()
		{
			return _start;
		}

		iterator end()
		{
			return _finish;
		}

		const_iterator begin() const
		{
			return _start;
		}

		const_iterator end() const
		{
			return _finish;
		}

		vector()
		{
		}

		vector(initializer_list<T> il)
		{
			reserve(il.size());
			for (const auto& e : il)
			{
				push_back(e);
			}
		}

		~vector()
		{
			if (_start)
			{
				delete[] _start;
				_start = _finish = _end_of_storage = nullptr;
			}
		}

		// 传统写法
		// v2(v1)
		//vector(const vector<T>& v)
		//{
		//	reserve(v.capacity());
		//	for (const auto& e : v)
		//	{
		//		push_back(e);
		//	}
		//}

		//// v0 = v1 = v3
		//// v1 = v3
		//vector<T>& operator=(const vector<T>& v)
		//{
		//	if (this != &v)
		//	{
		//		clear();
		//		reserve(v.capacity());
		//		for (const auto& e : v)
		//		{
		//			push_back(e);
		//		}
		//	}
		//	return *this;
		//}

		vector(size_t n, const T& val = T())
		{
			reserve(n);
			for (size_t i = 0; i < n; i++)
			{
				push_back(val);
			}
		}

		vector(int n, const T& val = T())
		{
			reserve(n);
			for (int i = 0; i < n; i++)
			{
				push_back(val);
			}
		}

		// 函数模板，迭代器不一定是vector迭代器，也可以是其他容器的迭代器
		template <class InputIterator>
		vector(InputIterator first, InputIterator last)
		{
			// reserve(last - first);
			while (first != last)
			{
				push_back(*first);
				++first;
			}
		}

		// 现代写法 8:11
		vector(const vector<T>& v)
		{
			vector<T> tmp(v.begin(), v.end());
			swap(tmp);
		}

		// v1 = v3
		vector<T>& operator=(vector<T> tmp)
		{
			swap(tmp);
			return *this;
		}

		void swap(vector<T>& v)
		{
			std::swap(_start, v._start);
			std::swap(_finish, v._finish);
			std::swap(_end_of_storage, v._end_of_storage);
		}

		void clear()
		{
			_finish = _start;
		}

		bool empty() const
		{
			return _start == _finish;
		}

		void reserve(size_t n)
		{
			if (n > capacity())
			{
				size_t sz = size();
				T* tmp = new T[n];
				if (_start)
				{
					for (size_t i = 0; i < sz; i++)
					{
						//tmp[i] = _start[i]; // 如果是string，调用string的赋值深拷贝
						std::swap(tmp[i], _start[i]);  // 如果是string，调用string的交换，交换资源指向
					}

					//memcpy(tmp, _start, sizeof(T) * sz);
					delete[] _start;
				}
				_start = tmp;
				_finish = _start + sz;
				_end_of_storage = _start + n;
			}
		}

		void resize(size_t n, T val = T())
		{
			if (n < size())
			{
				// 删除数据
				_finish = _start + n;
			}
			else
			{
				reserve(n);
				while (_finish < _start + n)
				{
					*_finish = val;
					++_finish;
				}
			}
		}

		size_t capacity() const
		{
			return _end_of_storage - _start;
		}

		size_t size() const
		{
			return _finish - _start;
		}

		T& operator[](size_t i)
		{
			assert(i < size());

			return _start[i];
		}

		const T& operator[](size_t i) const
		{
			assert(i < size());

			return _start[i];
		}

		void push_back(const T& x)
		{
			// 满了
			if (_finish == _end_of_storage)
			{
				reserve(capacity() == 0 ? 4 : capacity() * 2);
			}

			*_finish = x;
			++_finish;
		}

		void pop_back()
		{
			assert(!empty());
			--_finish;
		}

		iterator insert(iterator pos, const T& x)
		{
			assert(pos >= _start);
			assert(pos <= _finish);

			// 扩容
			if (_finish == _end_of_storage)
			{
				size_t len = pos - _start;
				reserve(capacity() == 0 ? 4 : capacity() * 2);
				pos = _start + len;
			}

			// 挪动数据
			iterator end = _finish - 1;
			while (end >= pos)
			{
				*(end + 1) = *end;
				--end;
			}

			*pos = x;
			++_finish;

			return pos;
		}

		iterator erase(iterator pos)
		{
			assert(pos >= _start);
			assert(pos < _finish);

			iterator it = pos + 1;
			while (it != _finish)
			{
				*(it - 1) = *it;
				++it;
			}

			--_finish;

			return pos;
		}

	private:
		iterator _start = nullptr;
		iterator _finish = nullptr;
		iterator _end_of_storage = nullptr;
	};
}