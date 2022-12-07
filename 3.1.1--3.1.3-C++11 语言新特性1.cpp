//C++11 语言新特性

#include "iostream"
#include <vector>
#include <list>
using namespace std;

void f(int)
{
	cout << "f int" << endl;
}

void f(void*)
{
	cout << "f void*" << endl;
}

double fun()
{
	return 0.0;
}

void print(initializer_list<int> vals);

int main()
{
	//微小但重要的语法提升
	{
		vector<list<int> > v1;  //OK in each C++ version
		vector<list<int>> v2;   //OK since C++11

		//nullptr 和 std::nullptr_t
		f(NULL);		//执行f(int)
		f(nullptr);		//执行f(void*)  
		//nullptr是关键字，它被自动转换为各种pointer类型,但不会转换为任何整数类型，
		//它拥有类型std::nullptr_t,定义与<cstddef>
		//std::nullptr_t 被视为一个基础类型
	}

	//auto 完成类型自动推导
	{
		auto i = 42; //i has type int	
		auto d = fun();//d has type double
		vector<string> v;
		auto pos = v.begin();//pos has type vector<string>::iterator

		auto l = [](int x)->bool {
			return x % 2 == 0;
		};
		//l has the type of a lambda 
		//taking an int and returning a bool
		bool b = l(4);
	}



	//一致性初始化与初值列
	{
		int values[]{ 1,2,3 };
		vector<int> v{ 2,3,5,7,11,13,17 };
		vector<string> cities{ "Berlin","New York","London","Braunschweig","Cairo","Cologne" };	

		int j{};	//j is initialized by 0
		int* q{};   //q is initialized by nullptr

		//void print(initializer_list<int> vals)
		print({ 1,2,3,4,5,6,7 });//pass a list of values to print()
		
	}

}

void print(initializer_list<int> vals)
{
	for (auto p = vals.begin();p!=vals.end();++p)
	{
		cout << *p << "\n";
	}
}
