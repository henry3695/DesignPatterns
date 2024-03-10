#ifndef CBIGN_H
#define CBIGN_H
#include <ostream>
using namespace std;
#define MAXN 1005
class CBigN
{
    int d[MAXN];
    int len;
public:
    CBigN();
	CBigN(char str[]);


    friend std::ostream& operator<<(std::ostream& os, const CBigN& b);
	bool   operator>(const CBigN & T)const;
	CBigN operator+(const CBigN & T) const;   //两个大数之间的相加运算 
	CBigN operator-(const CBigN & T) const;   //大数减小数
};

#endif // CBIGN_H
