/*
欧几里得算法证法一
a可以表示成a = kb + r（a，b，k，r皆为正整数，且r不为0）
假设d是a,b的一个公约数，记作d|a,d|b，即a和b都可以被d整除。
而r = a - kb，两边同时除以d，r/d=a/d-kb/d，由等式右边可知m=r/d为整数，因此d|r
因此d也是b,a mod b的公约数。
因(a,b)和(b,a mod b)的公约数相等，则其最大公约数也相等，得证。
*/

#include <iostream>
using namespace std;
int gcd(int a, int b)
{
	if (a % b == 0) 
		return b;
	else 
		return gcd(b, a % b);
}

int main() {

	int a, b,g;
	cin >> a >> b;

	g = gcd(a, b);
	cout << g<<endl;
	cout << (a*b) / g<<endl;

	return 0;
}
