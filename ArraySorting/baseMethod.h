#pragma once
#include <stdlib.h> 
#include <time.h>
#include <iostream>
using namespace std;

#define N_LEN  10
class CBaseMathod{
protected:
    int m_ary[N_LEN];

public:

    virtual void Sort_Array()=0;
    CBaseMathod()
    {
        srand((unsigned)time(NULL)); 
        for (int i = 0; i < N_LEN; i++)
        {
            m_ary[i]=i;
        }
        for (int i = 0; i < N_LEN; i++)
        {
            int nTmp =rand() % N_LEN;
            int nNum = m_ary[i] ;
            m_ary[i] = m_ary[nTmp];
            m_ary[nTmp] = nNum;
        }
    }
    void Print_Arry()
    {
         for (int i = 0; i < N_LEN; i++)
        {
            cout<<m_ary[i];
            if( (i+1)!=N_LEN )
            {
                cout<<",";
            }
            else
            {
                cout<<endl;
            }           
        }
    }
};