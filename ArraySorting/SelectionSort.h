#pragma once
#include "baseMethod.h"

/*
1. 算法步骤
首先在未排序序列中找到最小（大）元素，存放到排序序列的起始位置。
再从剩余未排序元素中继续寻找最小（大）元素，然后放到已排序序列的末尾。
重复第二步，直到所有元素均排序完毕。
*/
class CSelectionSort:public CBaseMathod
{
    public:
        void Sort_Array()
        {
            int i, j;
            for (i = 0; i < (N_LEN-1); i++)
            {
                int nMinIdx=i;
                for (j = (i+1); j < N_LEN; j++)
                {
                    if(m_ary[j]<m_ary[nMinIdx])
                    {
                        nMinIdx=j;
                    }
                }  

                swap(m_ary[i],m_ary[nMinIdx]);              

                cout<<"第"<<i+1<<"趟:";
                Print_Arry();
            }
        }


};