#include "cbign.h"
#include <string.h>

CBigN::CBigN()
{
    memset(d, 0, sizeof(d));
    len = 0;
}

CBigN::CBigN(char str[])
{
	memset(d, 0, sizeof(d));
	len = strlen(str);
	for (int i = 0; i < len; i++)
	{
		d[i] = str[len - 1 - i] - '0';
	}
}

bool CBigN::operator>(const CBigN & T) const
{
	if (len > T.len)
		return true;
	else if (len < T.len)
		return false;
	else {
		for (int i = len - 1; i >= 0; i--)
		{
			if (d[i] > T.d[i])
			{
				return true;
			}
			else if (d[i] < T.d[i])
			{
				return false;
			}
		}		
	}
	return false;
}


CBigN CBigN::operator+(const CBigN & T) const
{
	CBigN c;
	int carry = 0;//进位标记
	for (int i = 0; i < len || i < T.len; i++)
	{
		int tmp = d[i] + T.d[i] + carry;
		c.d[c.len++] = tmp % 10;
		carry = tmp / 10;
	}
	if (carry != 0)
	{
		c.d[c.len++] = carry;
	}
	return c;
}

CBigN CBigN::operator-(const CBigN & T) const
{
	CBigN c;
	int carry = 0;
	for (int i = 0; i < len || i < T.len; i++)
	{
		int tmp = d[i] - T.d[i] - carry;
		if (tmp < 0)
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

	while (c.len > 1 && c.d[c.len - 1] == 0)//去掉高位0
		c.len--;

	return c;
}

std::ostream& operator<<(std::ostream& os, const CBigN& b)
{
	for (int i = b.len - 1; i >= 0; i--)
	{
		os << b.d[i];
	}

	return os;
}

