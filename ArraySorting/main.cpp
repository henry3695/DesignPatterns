#include "Context.h"
#include <iostream>
using namespace std;

int main(){
    Context cb("冒泡排序");
    cb.Print_Arry();
    cb.Sort_Array();
    cb.Print_Arry();
    return 0;
}