#include "iostream"
using namespace std;


class MyClass
{
private:
	int a;
public:
	MyClass(int _a)
	{
		a = _a;
		cout << "MyClass()" << endl;
	}
	~MyClass()
	{
		cout << "~MyClass()" << endl;
	}

	MyClass(MyClass&& t) = delete;
	//MyClass(MyClass&& t)
	//{
	//	a = t.a;
	//}

	MyClass(const MyClass& t) = delete;       //禁用复制构造函数	
// 	MyClass(const MyClass& t)
// 	{
// 	a = t.a;
// 	}
	

	MyClass& operator=(MyClass& mc) = delete; //禁用赋值运算符
// 	MyClass& operator=(MyClass& mc)			  
// 	{
// 	a = mc.a;
// 	mc.a = 0;
// 	return *this;
// 	}

	MyClass& operator=(MyClass&& mc)
	{
		a = mc.a;
		mc.a = 0;
		return *this;
	}
};

int main()
{
	MyClass my1(1);
	MyClass my2(2);

	//MyClass my3(my1);		    //编译通不过  禁用复制构造函数
	//my2 = std::move(my1);   //编译通不过  禁用赋值运算符

	//MyClass my4(std::move(my2)); // error C2280: “MyClass::MyClass(const MyClass &)”: 尝试引用已删除的函数


	my2 = std::move(my1);
}
//C++ 禁用复制构造函数和赋值运算符

