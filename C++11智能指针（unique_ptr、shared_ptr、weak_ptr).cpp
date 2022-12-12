#include <iostream>
#include <memory>
using namespace std;
class myclass {
	int m_a;
public:
	myclass(int a)
	{
		m_a = a;
		cout << "this addr: 0x" << this<<"\tm_a=" << m_a << "\tmyclass(int a)" <<endl;
	}
	~myclass()
	{
		cout << "this addr: 0x"<< this<<"\tm_a=" << m_a << "\t~myclass" <<endl;
	}

	friend ostream& operator<<(ostream& os, const myclass &my)
	{
		os << "this addr: 0x" << &my << "\tm_a=" << my.m_a;
		return os;
	}	
};

void test_unique_ptr()
{
	std::unique_ptr<myclass> up1(new myclass(11));
	std::cout << *up1 << std::endl;			  // 11
    
	//std::unique_ptr<int> up2 = up1;		  // err, 不能通过编译
	std::unique_ptr<myclass> up3 = std::move(up1);// 现在p3是数据的唯一的unique_ptr

	std::cout << *up3 << std::endl;			  // 11
	//std::cout << *up1 << std::endl;		  // err, 运行时错误，空指针
	up3.reset();							  // 显式释放内存
	up1.reset();							  // 不会导致运行时错误
	//std::cout << *up3 << std::endl;         // err, 运行时错误，空指针

	std::unique_ptr<myclass> up4 = make_unique<myclass>(22);
	up4.reset(new myclass(44));					  // "绑定"动态对象
	std::cout << *up4 << std::endl;           // 44
	up4 = nullptr;							  // 显式销毁所指对象，同时智能指针变为空指针。与up4.reset()等价

	std::unique_ptr<myclass> up5(new myclass(55));
	myclass *p = up5.release();					  // 只是释放控制权，不会释放内存
	std::cout << *p << std::endl;
	//cout << *up5 << endl;				      // err, 运行时错误，不再拥有内存
	delete p;								  // 释放堆区资源

	return;
}

void test_shared_ptr()
{
	std::shared_ptr<myclass> sp1 = make_shared<myclass>(22);
	std::shared_ptr<myclass> sp2 = sp1;
	std::cout << "cout: " << sp2.use_count() << std::endl; // 打印引用计数, 2

	std::cout << *sp1 << std::endl;  // 22
	std::cout << *sp2 << std::endl;  // 22

	sp1.reset(); // 显示让引用计数减一
	std::cout << "count: " << sp2.use_count() << std::endl; // 打印引用计数, 1

	std::cout << *sp2 << std::endl; // 22

	return;
}


void check(std::weak_ptr<myclass> &wp) {
	std::shared_ptr<myclass> sp = wp.lock();  // 转换为shared_ptr<int>
	if (sp != nullptr) {
		std::cout << "still: " << *sp << std::endl;
	}
	else {
		std::cout << "still: " << "pointer is invalid" << std::endl;
	}
}


void test_weak_ptr()
{
	std::shared_ptr<myclass> sp1(new myclass(22));
	std::shared_ptr<myclass> sp2 = sp1;
	std::weak_ptr<myclass> wp = sp1;  // 指向shared_ptr<int>所指对象

	std::cout << "count: " << wp.use_count() << std::endl;  // count: 2
	std::cout << *sp1 << std::endl;  // 22
	std::cout << *sp2 << std::endl;  // 22
	check(wp);  // still: 22

	sp1.reset();
	std::cout << "count: " << wp.use_count() << std::endl;  // count: 1
	std::cout << *sp2 << std::endl;  // 22
	check(wp);  // still: 22

	sp2.reset();
	std::cout << "count: " << wp.use_count() << std::endl;  // count: 0
	check(wp);  // still: pointer is invalid

	return;
}

int main()
{
	test_unique_ptr();
	cout << "*********************************" << endl;
	test_shared_ptr();
	cout << "*********************************" << endl;
	test_weak_ptr();
	cout << "*********************************" << endl;
	return 0;
}
/*
this addr: 0x00000292C3904060   m_a = 11  myclass(int a)
this addr : 0x00000292C3904060   m_a = 11
this addr : 0x00000292C3904060   m_a = 11
this addr : 0x00000292C3904060   m_a = 11  ~myclass
this addr : 0x00000292C3903960   m_a = 22  myclass(int a)
this addr : 0x00000292C3903FA0   m_a = 44  myclass(int a)
this addr : 0x00000292C3903960   m_a = 22  ~myclass
this addr : 0x00000292C3903FA0   m_a = 44
this addr : 0x00000292C3903FA0   m_a = 44  ~myclass
this addr : 0x00000292C3903DA0   m_a = 55  myclass(int a)
this addr : 0x00000292C3903DA0   m_a = 55
this addr : 0x00000292C3903DA0   m_a = 55  ~myclass
*********************************
this addr : 0x00000292C3904C60   m_a = 22  myclass(int a)
cout : 2
this addr : 0x00000292C3904C60   m_a = 22
this addr : 0x00000292C3904C60   m_a = 22
count : 1
this addr : 0x00000292C3904C60   m_a = 22
this addr : 0x00000292C3904C60   m_a = 22  ~myclass
*********************************
this addr : 0x00000292C3904520   m_a = 22  myclass(int a)
count : 2
this addr : 0x00000292C3904520   m_a = 22
this addr : 0x00000292C3904520   m_a = 22
still : this addr : 0x00000292C3904520    m_a = 22
count : 1
this addr : 0x00000292C3904520   m_a = 22
still : this addr : 0x00000292C3904520    m_a = 22
this addr : 0x00000292C3904520   m_a = 22  ~myclass
count : 0
still : pointer is invalid
*********************************
*/
