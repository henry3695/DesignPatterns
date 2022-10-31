#include "Context.h"
#include <iostream>
using namespace std;

int main(){
    Context cb("冒泡排序");
    cb.Print_Arry();
    cb.Sort_Array();
    cb.Print_Arry();


    Context cb1("选择排序");
    cb1.Print_Arry();
    cb1.Sort_Array();
    cb1.Print_Arry();

    Context cb2("插入排序");
    cb2.Print_Arry();
    cb2.Sort_Array();
    cb2.Print_Arry();
    return 0;
}