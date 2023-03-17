// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include<chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <future>
#define  MYLOG(str)			std::cout <<"[0x"<<std::setw(8)<<std::setfill('0')<<std::this_thread::get_id()<<"] "<<str<<std::endl
#define  CLASSLOG(str)		std::cout <<"[0x"<<std::setw(8)<<std::setfill('0')<<std::this_thread::get_id()<<"] "<<"[0x"<<this<<"] "<<str<<std::endl
void fun()
{
	MYLOG("func()  1");
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	MYLOG("func() 2");
}
int main()
{
	{
		//std::thread t(fun);  //会报错，因为t析构了，线程没退出
		std::async(fun);       //不会报错
	}


	std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	MYLOG("main()");
	return 0;
}
