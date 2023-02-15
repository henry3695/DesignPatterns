#include "iostream"
#include "thread"
#include "mutex"
#include "string"
using namespace std;

//死锁发生情况

//一，忘记释放锁
#if 0
mutex _mutex;
void func(string str)
{
	_mutex.lock();
	cout << "lock.."<<str;
}
int main() {
	thread t1(func,"t1");
	t1.join();


	thread t2(func, "t2");
	t2.join();
}
#endif

//二，单线程重复申请锁
#if 0
mutex _mutex;
void func()
{
	cout << "func() 1 " << endl;
	_mutex.lock();
	cout << "func() 2" << endl;
	_mutex.unlock();
}

void data_process() {
	_mutex.lock();
	func();
	_mutex.unlock();
}
int main() {
	thread t1(data_process);
	t1.join();
}
#endif


//三，双线程多锁申请
#if 1
mutex _mutex1;
mutex _mutex2;

void process1() {
	_mutex1.lock();
	cout << "process11()" << endl;  //执行完这一步   等线程2中的_mutex2解锁
	_mutex2.lock();
	cout << "process12()" << endl;
	_mutex2.unlock();
	_mutex1.unlock();
}

void process2() {
	_mutex2.lock();
	cout << "process21()" << endl;  //执行完这一步   等线程1中的_mutex1解锁
	_mutex1.lock();
	cout << "process22()" << endl;
	//do something2...
	_mutex1.unlock();
	_mutex2.unlock();
}

int main() {
	thread t1(process1);
	thread t2(process2);

	t1.join();
	t2.join();

	return 0;
}
#endif

/*
解决死锁：
一，尽量避免同时只对一个互斥锁上锁
二，互斥锁保护区域不要使用操作其他互斥锁的代码。
三，如果想同时对多个互斥锁上锁，要使用std::lock()
四，给锁定义顺序（使用层次锁，或者比较地址等），每次以同样的顺序进行上锁。
原则
如果另外一个线程可能会依赖当前线程，则不要再让当前线程依赖那个线程了

*/
