#include <iostream>
#include <string>
using namespace std;


/*
C++ 中常见的几种输入字符串的方法如下：

cin、cin.get()、cin.getline()、getline()、gets()、getchar()
*/
int main1()
{
    string str;
    str="test";
    getline(cin,str);

    const char* p = str.c_str();
    str[1]='\0';   
    cout<<str<<endl;
    cout<<str.c_str()<<endl;
    cout<<p<<","<<str.size()<<","<<str.length();

    return 0;
}
/*
输入
testab

输出
t stab
t
t,6,6
*/

//用法一：最常用、最基本的用法，输入一个数字：
//#include <iostream>
//using namespace std;
int main2 ()
{
  int a,b;
  cin>>a>>b;
  cout<<a+b<<endl;

  return 0;
}
//输入：2[回车]3[回车]
//输出：5


//用法二：接受一个字符串，遇“空格”、“Tab”、“回车”都结束,有越界风险
int main3 ()
{
  char a[10];
  cin>>a;
  cout<<a<<endl;
  return 0;
}
//输入：jkljkljkl
//输出：jkljkljkl
//输入：jkljkl jkljkl
//遇空格结束，所以不能输入多个单词
//输出：jkljkl


//2. cin.get()
//用法一：cin.get(字符变量名)可以用来接收字符
int main4 ()
{
    char ch;
    ch=cin.get(); //或者cin.get(ch);只能获取一个字符
    cout<<ch<<endl;
    return 0;
}

//输入：jljkljkl
//输出：j


//用法二：cin.get(字符数组名，接收字符数)用来接收一行字符串，可以接收空格

int main5()
{
    char a[20];
    cin.get(a,20); //有些类似getline。可以输入多个单词，中间空格隔开。
    cout<<a<<endl;
    return 0;
}
//输入：jkl jkl jkl
//输出：jkl jkl jkl
//输入：abcdeabcdeabcdeabcdeabcde （输入25个字符）
//输出：abcdeabcdeabcdeabcd （接收19个字符+1个'\0'）


//3.cin.getline()
//cin.getline(): 接受一个字符串，可以接收空格并输出

//#include <iostream>
//using namespace std;
int main6 ()
{
    char m[20];
    cin.getline(m,5); //与上面基本相同。
    cout<<m<<endl;
    return 0;
}

//输入：jkljkljkl
//输出：jklj
/*接受5个字符到m中，其中最后一个为'\0'，所以只看到4个字符输出；​

如果把5改成20：

输入：jkljkljkl
输出：jkljkljkl
​
输入：jklf fjlsjf fjsdklf
输出：jklf fjlsjf fjsdklf
*/


/*
4. getline()
getline() ：接受一个字符串，可以接收空格并输出，需包含 #include<string>。

#include<iostream>
#include<string>
using namespace std;
*/
int main7 ()
{
    string str;
    getline(cin,str);
    cout<<str<<endl;
    return 0;
}
/*
​
测试：

输入：jkljkljkl //VC6中有个bug,需要输入两次回车。
输出：jkljkljkl
​
输入：jkl jfksldfj jklsjfl
输出：jkl jfksldfj jklsjfl

*/

/*
5. gets()
gets()： 接受一个字符串，可以接收空格并输出，需包含 #include<string>。有越界风险

#include<iostream>
#include<string>
using namespace std;

*/

#include <vector>

void test()
{
    cout<<32;
}
int main11 ()
{
    //char m[20];
    //gets(m); //不能写成m=gets();
    //cout<<m<<endl;

    //vector<int> vec;
    //vec.push_back

    cout<<hex<<12345678<<endl;

    cout<<dec<<16<<endl;

    test();
    return 0;
}
/*

​测试：

输入：jkljkljkl
输出：jkljkljkl
​
输入：jkl jkl jkl
输出：jkl jkl jkl
*/
