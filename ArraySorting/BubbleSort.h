#pragma once
#include "baseMethod.h"

/*
1. 算法步骤
比较相邻的元素。如果第一个比第二个大，就交换他们两个。
对每一对相邻元素作同样的工作，从开始第一对到结尾的最后一对。这步做完后，最后的元素会是最大的数。
针对所有的元素重复以上的步骤，除了最后一个。
持续每次对越来越少的元素重复上面的步骤，直到没有任何一对数字需要比较。
*/
class CBubbleSort:public CBaseMathod
{
    public:
        void Sort_Array()
        {
            int i, j;
            for (i = 0; i < N_LEN - 1; i++)
            {
                for (j = 0; j < N_LEN - 1 - i; j++)
                {
                    if (m_ary[j] > m_ary[j + 1])
                        swap(m_ary[j], m_ary[j + 1]);            
                }

                cout<<"第"<<i+1<<"趟:";
                Print_Arry();
            }
        }


};