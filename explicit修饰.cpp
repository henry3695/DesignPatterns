//构造函数被explicit修饰后, 就不能再被隐式调用了

#include <iostream>
using namespace std;

class P {
public:
	P(int a, int b)
	{
		cout << a << b << endl;
	}
	explicit P(int a, int b,int c)
	{
		cout << a << b <<c<< endl;
	}
};

void fp(const P&)
{

}
int main()
{
	P x(77, 5);//OK
	P y(77, 5);//OK
	P z{ 77,5,42 };//OK
	P v = { 77,5 };//OK
	//P w = { 77,5,42 }; ERROR

	fp({ 47,11 });
	//fp({ 47,11,3 }); ERROR due to explicit

	fp(P{ 47,11 });
	fp(P{ 47,11,3 });
}
