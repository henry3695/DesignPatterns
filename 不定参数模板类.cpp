#include <iostream>
#include <bitset>
using namespace std;
void print()
{

}
template<typename T,typename... Types>
void print(const T& firstArg,const Types&... args)
{
    //cout<<sizeof...(args)<<endl;
    cout<<firstArg<<endl;
    print(args...);
}
/*
template<typename... Types>
void print(const Types&... args)
{
    cout<<"aa"<<endl;
}
*/

int main()
{
	print(7.5,"hello",bitset<16>(377),42);
	
	return 0;
}
