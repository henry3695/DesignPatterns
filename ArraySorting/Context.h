#pragma once
#include "baseMethod.h"
#include "BubbleSort.h"
#include <string>

class  Context
{
private:
    CBaseMathod* m_pMathod;
public:
     Context(string orderType);
    ~ Context();

    void Sort_Array();
    void Print_Arry();
};

Context::Context(string orderType)
{
    m_pMathod=NULL;
    if(orderType=="冒泡排序")
    {
        m_pMathod = new CBubbleSort();
    }

}
void Context::Sort_Array()
{
   if(m_pMathod!=NULL)
    {
        m_pMathod->Sort_Array();
    } 
}
Context::~Context()
{
    if(m_pMathod!=NULL)
    {
        delete m_pMathod;
    }
}

void Context::Print_Arry()
{
 if(m_pMathod!=NULL)
    {
        m_pMathod->Print_Arry();
    }
}