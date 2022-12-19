#include <thread>
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std;

#define  MYLOG(str)			cout <<"[0x"<<setw(8)<<setfill('0')<<std::this_thread::get_id()<<"] "<<str<<endl
#define  CLASSLOG(str)		cout <<"[0x"<<setw(8)<<setfill('0')<<std::this_thread::get_id()<<"] "<<"[0x"<<this<<"] "<<str<<endl

/*
//方法一、简单开启线程
void hello()
{
	MYLOG("Hello  Concurrent World!");
}
int main()
{
	cout << hex;
	MYLOG("main thread");
	thread t(hello);
	t.join();	
	return 0;
}
*/


/*
//方法二、lambda 方法
int main()
{
	cout << hex;
	MYLOG("main thread");

	thread t([] {
		MYLOG("Hello  Concurrent World!");
	});
	t.join();
	return 0;
}
*/

/*
//方法三、对象
class background_task
{
public:
	background_task()
	{
		CLASSLOG("background_task()");
	}
	~background_task()
	{
		CLASSLOG("~background_task()");
	}
	void operator()()const
	{
		CLASSLOG("Hello  Concurrent World!");
	}
};

int main()
{
	cout << hex;
	MYLOG("main thread");

	{
#if 0
		background_task f;
		thread t(f);
		t.join();
#endif

#if 0
		thread t((background_task()));
		t.join();
#endif

		thread t{ background_task() };
		t.join();
	}
	return 0;
}
*/


//2.2 传递参数给线程函数
//thread t(函数名,参数1,参数2);
class String
{
	char* m_pBuff;
	int   m_nLen;
public:
	String()
	{
	}
	String(const char* p)
	{
		m_nLen = strlen(p)+1;
		m_pBuff = new char[m_nLen];
		strcpy(m_pBuff, p);
		CLASSLOG("String(const char* p)");
	}

#if 0
	//String B(move(A)); 编译出错，删除移动构造函数
	String(String&& A) = delete;

	//C = move(B);   编译出错，删除赋值移动函数
	String& operator=(String&& A) = delete; 
//#else
	//移动构造函数
	String(String&& A) 
	{
		m_nLen = A.m_nLen;
		m_pBuff = A.m_pBuff;

		A.m_nLen = 0;
		A.m_pBuff = NULL;
		CLASSLOG("String(String&& A)");
	}

	//移动赋值函数
	String& operator=(String&& A)
	{
		CLASSLOG("String& operator=(String&& A)");

		if (this == &A)
			return *this;
		m_nLen = A.m_nLen;
		m_pBuff = A.m_pBuff;

		A.m_nLen = 0;
		A.m_pBuff = NULL;
		return *this;
	}
#endif
	~String()
	{
		CLASSLOG("~String()");
		if (m_pBuff)
		{
			delete[] m_pBuff;
			m_pBuff = NULL;
		}
		m_nLen = 0;
	}
	friend ostream& operator<< (ostream& os, const String& c)
	{
		MYLOG("ostream& operator<<");
		os << c.m_pBuff;
		return os;
	}
};
void f1(int i, const String & s)
{
	ostringstream ss;
	ss << i << s;
	MYLOG(ss.str());
}

void f2(int i, String & s)
{
	ostringstream ss;
	ss << i << s;
	MYLOG(ss.str());
}

class X
{
public:
	void do_lengthy_work(int a)
	{
		CLASSLOG("void do_lengthy_work(int a)");
		CLASSLOG(a);
	}
};

void process_big_object(std::unique_ptr<String> a)
{
	MYLOG(*a);
}

class Point
{

};
int main()
{
	cout << hex;
	MYLOG("main thread");
	{
		//空类可以支持，移动构造，移动赋值，调用了默认的移动构造函数，默认的移动赋值函数，但是都是空函数，啥也不做
		Point A;
		Point B(A);
		Point C;
		C = move(B);
	}

	{
		String A("test");
		String B(move(A));
		String C;
		C = move(B);
	}
#if 0
	{
		//thread t(函数名,参数1,参数2);
		thread t(f1, 1, "hello");
		t.join();
	}

	{
		/*
		String str("hello");
		thread t(f2, 2,str); //会析构两次程序会报错
		t.join();
		*/

		//String str("hello");
		//thread t(f2, 2,std::ref(str)); //正确调用,传递引用
		//t.join();

		X my_x;
		thread t(&X::do_lengthy_work, &my_x, 100);//t(成员函数地址，类实例，第一个参数,....)
		t.join();
	}
#endif

	{
		unique_ptr<String> p(new String("test prt"));
		thread t(process_big_object, move(p));
		t.join();
	}
	return 0;
}

