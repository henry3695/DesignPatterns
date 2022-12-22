#include <thread>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;

#define  MYLOG(str)			cout <<"[0x"<<setw(8)<<setfill('0')<<std::this_thread::get_id()<<"] "<<str<<endl
#define  CLASSLOG(str)		cout <<"[0x"<<setw(8)<<setfill('0')<<std::this_thread::get_id()<<"] "<<"[0x"<<this<<"] "<<str<<endl

/*
什么是 Lambda 表达式？
Lambda 表达式是 C++11 提出的新特性，主要用来实现代码中函数的内嵌，简化了编程，提高了效率。

它的基本格式如下：
auto fun = [捕获参数](函数参数){函数体};
*/


class MyClass {
protected:
	int num;
public:
	MyClass():num(10)
	{
	}
	void function() {
		// 使用 [this] 来捕获 this
		auto fun1 = [this](int v) {cout << v + this->num << endl; this->num = 7; };
		// 使用 [&] 捕获所有父作用域的引用，包括 this
		auto fun2 = [&](int v) {cout << v + this->num << endl; };

		fun1(5);//15
		fun2(6);//13
	}
};

int main()
{
	{
		auto fun = []() { std::cout << "Hello Lambda" << std::endl; };
		fun();


		//Function point
		[]() {cout << "Hello World!" << endl; };//后面不加 () 的 Lambda 表达式相当于函数指针

		//Call function
		[]() {cout << "Hello World!" << endl; }();//加上 () 就相当于调用这个 Lambda 函数。
	}


	{
		int num = 100;
		// 为 Lambda 定义一个 int 类型的参数
		auto fun = [](int num) { num = 5; cout << num << endl; };//5  修改的只是临时变量
		fun(num);
		
		cout << num << endl;// num = 100
	}

	{
		// 1. 返回 `a + b` 的类型
		int num1 = [](int a, int b) { return a + b; }(1, 2);
		// 2. 返回值转换成 int 类型
		int num2 = [](double a, double b)->int { return a + b; }(1.2, 2.1);
		// 3. 自动推导返回值的类型
		double num3 = [](double a, double b)->decltype(a + b) { return a + b; }(1.2, 2.1);
		cout << num1 << endl;
		cout << num2 << endl;
		cout << num3 << endl;
	}

	{
		//[=]：捕获的局部变量只可读不可写，捕获范围是当前 Lambda 表达式之前的作用域。
		//[&]：捕获的局部变量可读可写。

		int num = 100;
		// read only 
		auto fun1 = [=]() { cout << num << endl; }; //100
		fun1();

		// read and write
		auto fun2 = [&num]() { num = 200; cout << num << endl; };//200
		fun2();

		cout << num << endl;//200
	}

	{
		//Lambda 默认是 const 函数，不能修改引用的变量，使用 mutable 可以取消该属性，但是 mutable 修改的只是副本。
		int num = 100;

		//Lamnda is const function, use `mutable` can cancel const
		//auto fun = [=](){ num = 200; cout << num << endl; };
		auto fun = [=]() mutable { num = 200; cout<<&num<<"," << num << endl; };
		fun();
		// num = 100
		cout << &num << ","<<  num << endl;


		auto fun1 = [&]() { num = 200; cout << &num << ","<<  num << endl; };
		fun1();
		cout << &num << "," << num << endl;

	}

	{
		string str;
		auto fun = [](string str) {cout << str << endl; };		
		fun("this is test");
		//lambda表达式没有地址，是内联展开的
		//cout << (void*)fun << endl; //编译出错
	}


	{
		MyClass m;
		m.function();
	}

	{
		vector<int> vec = { 1, 2, 3, 4, 5 };
		// 定义 Lambda
		auto fun = [](int num) { cout << num << endl; };
		for_each(vec.begin(), vec.end(), fun);

		// 将 vec 中的元素都加 1，修改参数必须传递参数的引用
		for_each(vec.begin(), vec.end(), [](int& num) { num += 1; });
		for_each(vec.begin(), vec.end(), fun);
	}

	{
		int x = 1;
		auto valueLambda = [=]() { cout << x << endl; };
		auto refLambda = [&]() { cout << x << endl; };
		x = 13;
		valueLambda();// 1
		refLambda();  // 13
	}
	
	
	return 0;
}








