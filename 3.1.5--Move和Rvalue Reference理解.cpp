#include <iostream>
#include <string>
#include <vector>
using namespace std;
class X
{
	int m_n;
public:
	friend ostream& operator<< (ostream& ct,const X& x)
	{
		ct << x.m_n;
		return ct;
	}
	X(const int n)
	{
		this->m_n = n;
		cout <<"this addr:0x"<<this<<" X(const int n)"<<endl;
	}
	X(const X& lvalue)
	{
		this->m_n = lvalue.m_n;
		cout << "this addr:0x" << this << " X(const X& lvalue)" << endl;
	}
	X(X&& lvalue)
	{		
		this->m_n = lvalue.m_n;
		lvalue.m_n = -1;
		cout << "this addr:0x" << this << " X(X&& lvalue)" << endl;
	}
	~X()
	{
		cout << "this addr:0x" << this << " ~X()" << endl;
	}

	X& operator=(const X& lvalue)
	{
		this->m_n = lvalue.m_n;
		cout << "X& operator=(const X& lvalue)" << endl;
		return *this;
	}
	X& operator=(X&& rvalue)
	{
		this->m_n = rvalue.m_n;
		rvalue.m_n = -1;
		cout << "X& operator=(X&& rvalue)" << endl;
		return *this;
	}	

	
};
int main()
{
	{
		cout << hex;
		vector<X> v1{ X(1),X(2), X(3), X(4) };

		X x2(std::move(v1[0]));
		cout << endl<<"*******************************" << endl;
		for (auto& e:v1)
		{
			cout << (int)&e <<":"<<e<< endl;
		}
		cout << endl << "*******************************" << endl;

		
		vector<X> v2 = std::move(v1);
		for (int i=0;i<v2.size();i++)
		{
			X& f = v2[i];
			cout << (int)&f << ":" << f << endl;
		}
		cout << endl << "*******************************" << endl;
	}


	return 0;
}

/*
this addr:0x006FFA2C X(const int n)
this addr:0x006FFA30 X(const int n)
this addr:0x006FFA34 X(const int n)
this addr:0x006FFA38 X(const int n)
this addr:0x009FF7B0 X(const X& lvalue)
this addr:0x009FF7B4 X(const X& lvalue)
this addr:0x009FF7B8 X(const X& lvalue)
this addr:0x009FF7BC X(const X& lvalue)
this addr:0x006FFA38 ~X()
this addr:0x006FFA34 ~X()
this addr:0x006FFA30 ~X()
this addr:0x006FFA2C ~X()
this addr:0x006FFB90 X(X&& lvalue)

*******************************
9ff7b0:ffffffff
9ff7b4:2
9ff7b8:3
9ff7bc:4

*******************************
9ff7b0:ffffffff
9ff7b4:2
9ff7b8:3
9ff7bc:4

*******************************
this addr:0x009FF7B0 ~X()
this addr:0x009FF7B4 ~X()
this addr:0x009FF7B8 ~X()
this addr:0x009FF7BC ~X()
this addr:0x006FFB90 ~X()

*/