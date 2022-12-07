//构造函数被explicit修饰后, 就不能再被隐式调用了

#include <iostream>
#include <vector>
#include <string>
using namespace std;

class P {
public:
	P(int a, int b)
	{
		cout << a << b << endl;
	}
	explicit P(int a, int b, int c)
	{
		cout << a << b << c << endl;
	}
};

void fp(const P&)
{

}


class C
{
private:
	string ms;
public:
	C(const std::string& s) //构造函数加了 explicit 就编译不过
	{
		ms = s;
	}
	friend ostream& operator << (ostream& os,const C& cc)
	{
		os << cc.ms;
		return os;
	}
};
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

	std::vector<std::string> vs;
	for (const C& elem : vs)
	{
		std::cout << elem << std::endl;
	}
}
