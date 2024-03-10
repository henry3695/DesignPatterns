/*
构造函数作用是对对象进行初始化，在堆上new一个对象或在栈上定义一个临时对象时，会自动调用对象的构造函数。
有初始化列表和构造函数体内赋值两种方式，

初始化列表在初始化对象时更高效（每个成员在初始化列表中只能出现一次），减少了一次赋值操作，推荐此方法；
*/
#include "iostream"
using namespace std;

class A
{
public:
	A()
	{
		cout << "A()" << endl;
	}
	~A()
	{
		cout << "~A()" << endl;
	}

	A& operator=(const A& _t)
	{
		cout << "=" << endl;
		return *this;
	}
};

class MyClass
{
private:
	A a;
public:
	MyClass(const A& t):a(t)  //初始化列表
	{
		cout << "MyClass()" << endl;
	}

/*  比上面多调用一次A的构造函数
	MyClass(const A& t)   //构造函数体内赋值
	{
		a = t;
		cout << "MyClass()" << endl;
	}
	*/
	~MyClass()
	{
		cout << "~MyClass()" << endl;
	}	
	
};

int main()
{
	A a;
	MyClass my(a);
}

