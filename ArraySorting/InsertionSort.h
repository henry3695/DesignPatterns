#pragma once
#include "baseMethod.h"

/*
1. 算法步骤
将第一待排序序列第一个元素看做一个有序序列，把第二个元素到最后一个元素当成是未排序序列。
从头到尾依次扫描未排序序列，将扫描到的每个元素插入有序序列的适当位置。
（如果待插入的元素与有序序列中的某个元素相等，则将待插入元素插入到相等元素的后面。）
*/
class CInsertionSort:public CBaseMathod
{
    public:
        void Sort_Array()
        {
             for(int i=1;i<N_LEN;i++)
             {
                int key=m_ary[i];
                int j=i-1;
                while((j>=0) && (key<m_ary[j]))
                {
                        m_ary[j+1]=m_ary[j];
                        j--;
                }
                m_ary[j+1]=key;
                cout<<"第"<<i+1<<"趟:";
                Print_Arry();
            }
        }


};