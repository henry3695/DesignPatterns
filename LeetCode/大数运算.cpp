
#include<chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <algorithm>

#define  MYLOG(str)			std::cout <<"[0x"<<std::setw(8)<<std::setfill('0')<<std::this_thread::get_id()<<"] "<<str<<std::endl
#define  CLASSLOG(str)		std::cout <<"[0x"<<std::setw(8)<<std::setfill('0')<<std::this_thread::get_id()<<"] "<<"[0x"<<this<<"] "<<str<<std::endl


const int MAXN = 1005;

struct bign
{
	int d[MAXN], len;
	bign() {
		memset(d, 0, sizeof(d));
		len = 0;
	}

	friend std::ostream& operator<<(std::ostream& os, const bign& b)
	{
		for (int i = b.len-1; i >=0;i--)
		{
			os << b.d[i];
		}

		return os;
	}
};

bign change(char str[])
{
	bign a;
	a.len = strlen(str);
	for (int i = 0; i < a.len;i++)
	{
		a.d[i] = str[a.len - 1 - i] - '0';
	}
	return a;
}

int compare(bign a, bign b) {
	if (a.len > b.len)
		return 1;
	else if (a.len < b.len)
		return -1;
	else {
		for (int i = a.len - 1; i >= 0; i--)
		{
			if (a.d[i]>b.d[i])
			{
				return 1;
			}
			else if (a.d[i]<b.d[i])
			{
				return -1;
			}
		}
		return 0;
	}
}
bign add(bign a, bign b)
{
	bign c;
	int carry = 0;//进位标记
	for (int i = 0; i < a.len || i < b.len;i++)
	{
		int tmp = a.d[i] + b.d[i] + carry;
		c.d[c.len++] = tmp % 10;
		carry = tmp / 10;
	}
	if (carry!=0)
	{
		c.d[c.len++] = carry;
	}
	return c;
}

bign sub(bign a, bign b)
{
	bign c;
	int carry = 0;
	for (int i = 0; i < a.len || i < b.len;i++)
	{
		int tmp = a.d[i] - b.d[i] - carry;
		if (tmp<0)
		{
			tmp += 10;
			carry = 1;
		}
		else
		{
			carry = 0;
		}
		c.d[c.len++] = tmp;
	}

	while (c.len>1 && c.d[c.len - 1] == 0)//去掉高位0
		c.len--;

	return c;
}

bign mul(bign a, int b)
{
	bign c;
	int carry = 0;
	for (int i = 0; i < a.len; i++)
	{
		int tmp = a.d[i] * b + carry;
		c.d[c.len++] = tmp % 10;
		carry = tmp / 10;
	}

	while (carry!=0)
	{
		c.d[c.len++] = carry % 10;
		carry /= 10;
	}
	return c;
}

bign factorial(int n)
{
	bign t;
	t.d[0] = 1;
	t.len = 1;
	for (int i = 1; i <= n;i++)
	{
		t=mul(t, i);
	}

	return t;
}




int main()
{
	bign t = factorial(100);
	std::cout << t;
}
