#define _CRT_SECURE_NO_WARNINGS 1 
#include<assert.h>
#include<string.h>

namespace lvx {
	class string {

	public:
		//迭代器
		typedef char* iterator;
		typedef const char* const_iterator;

		iterator begin() {
			return _str;
		}

		iterator end(){ return _str + _size; }

		const_iterator begin() const {
			return _str;
		}

		void reserve(size_t n);

		const_iterator end() const { return _str + _size; }
		//构造
		string();
		string(const char* str);
		//析构
		~string();
		//下标访问
		char& operator[](size_t n) { assert(n >= 0 && n <= _size); return _str[n]; }
		const char& operator[](size_t n) const { assert(n >= 0 && n <= _size); return _str[n]; }
		//size
		size_t size() { return _size; }
		//c_str
		char* c_str() { return _str; }
		//+= push_back append
		void push_back(char& c);
		void append(const char* c);
		string& operator+=(const char* c);
	private:
		char* _str;
		size_t _size;
		size_t _capacity;
	};
}

lvx::string::string(){
	_str = new char[1] {'\0'};
	_size = _capacity = 0;
}

lvx::string::string(const char* str)
	:_size(strlen(str))//sizeof会算上\0
{
	_str = new char[_size + 1];
	strcpy(_str, str);
	_capacity = _size;
}

lvx::string::~string(){
	delete[] _str;
	_str = nullptr;
	_size = _capacity = 0;
}

void lvx::string::reserve(size_t n) {
	if ( n > _capacity)
	{
		char* str = new char[n + 1];
		strcpy(str, _str);
		delete[] _str;
		_str = str;
		_capacity = n;
	}
}

void lvx::string::push_back(char& c)
{
	if (_size == _capacity)
	{
		reserve(_size + 1);
	}
	_str[_size] = c;
	_size++;
	_str[_size] = '\0';
}

void lvx::string::append(const char* c)
{
	size_t len = (strlen(c));
	if (_size + len > _capacity)
	{
		reserve(_size + len);
	}
	strcpy(_str + _size, c);
	_size += len;
}

lvx::string& lvx::string::operator+=(const char* c)
{
	append(c);
	return *this;
}