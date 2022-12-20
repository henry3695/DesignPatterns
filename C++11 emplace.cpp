#include <thread>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
using namespace std;

#define  MYLOG(str)			cout <<"[0x"<<setw(8)<<setfill('0')<<std::this_thread::get_id()<<"] "<<str<<endl
#define  CLASSLOG(str)		cout <<"[0x"<<setw(8)<<setfill('0')<<std::this_thread::get_id()<<"] "<<"[0x"<<this<<"] "<<str<<endl
/*
void f()
{

}
int main()
{
	//返回一个对于给定程序执行时能够正真并发运行的线程数量的指示
	cout << std::thread::hardware_concurrency() << endl;

	//线程ID  方法一
	cout << std::this_thread::get_id() << endl;

	//线程ID  方法二
	thread t(f);
	cout << t.get_id() << endl;
	t.join();
	return 0;
}
*/


using namespace std;

class Test
{
public:
	Test()
	{
		m_p = new int(100);
		CLASSLOG("Test()  m_p = new int(100);");
		CLASSLOG(m_p);
	}

	Test(int t)
	{
		m_p = new int(100);
		CLASSLOG("Test(int t)  m_p = new int(100);");
		CLASSLOG(m_p);
	}


	Test(Test &&test) //移动构造函数
	{
		m_p = test.m_p;
		test.m_p = nullptr; //修改参数的资源


		CLASSLOG("Test(Test &&test)");
	}
	Test &operator=(Test &&test) //移动赋值操作符
	{
		CLASSLOG("Test &operator=(Test &&test)");
		if (this != &test)
		{
			m_p = test.m_p;
			test.m_p = nullptr; //修改参数资源
		}
		return *this;
	}
	~Test()
	{
		if (m_p!=nullptr)
		{
			CLASSLOG(m_p);
			delete m_p;
			m_p = nullptr;
		}
		CLASSLOG("~Test()");
	}
private:
	int *m_p;
};


int main()
{
	if(1)
	{
		std::vector<Test> vec;
		vec.push_back(Test()); //构造两次
		vec.emplace(vec.begin(), Test()); //构造两次
		vec.emplace(vec.end(), 1); //构造一次
	}


	{
		//Test foo = Test();  //与Test foo;  效果一样
	}

	{
		
	}
	return 0;
}
