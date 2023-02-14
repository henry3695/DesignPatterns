#include "TickerQueue.h"
#include <QTime>
TickerQueue::TickerQueue(int nMaxSpan,bool bFree):m_nMaxSpan(nMaxSpan),m_bFree(bFree)
{
    m_lBuySz=0.0;
    m_lSellSz=0.0;
    m_lTotalSz=0.0;
    m_pHeadQueue=NULL;
    m_nSize=0;
}

void TickerQueue::AddNode(TickerData* pTickerData)
{
    m_nSize++;
    m_lTotalSz+=pTickerData->lastSz;
    if(pTickerData->bShowPer)
    {
        if(pTickerData->bSell)
        {
            m_lSellSz+=pTickerData->lastSz;
        }
        else
        {
            m_lBuySz+=pTickerData->lastSz;
        }
    }

    pTickerData->pNext = m_pHeadQueue;
    m_pHeadQueue=pTickerData;
}

void TickerQueue::CheckNode()
{
    qint64 TickNow =   GetTickCount();
    TickerData* pTickerData=m_pHeadQueue;
    TickerData* pLastNode = NULL;
    while (pTickerData) {
        if( (TickNow-pTickerData->tickTime)>m_nMaxSpan )
            break;

        pLastNode=pTickerData;
        pTickerData=pTickerData->pNext;
    }

    if(pTickerData==NULL)
        return;

    pLastNode->pNext=NULL;

    TickerData* pDelete=NULL;
    while (pTickerData) {
        //释放节点
       pDelete = pTickerData;

       m_nSize--;
       m_lTotalSz-=pDelete->lastSz;
       if(pDelete->bShowPer)
       {
           if(pDelete->bSell)
           {
               m_lSellSz-=pDelete->lastSz;
           }
           else
           {
               m_lBuySz-=pDelete->lastSz;
           }
       }
       delete pDelete;

       pTickerData=pTickerData->pNext;
    }
}


double   TickerQueue::GetTotalSz()
{
    return m_lTotalSz;
}

int   TickerQueue::GetSize()
{
    return m_nSize;
}

qint64 TickerQueue::GetTickCount()
{
    qint64 timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000;
    return timestamp;
}

double   TickerQueue::GetBuySz()
{
    return m_lBuySz;
}
double   TickerQueue::GetSellSz()
{
    return m_lSellSz;
}

double   TickerQueue::Percentage()
{
    double ltt =  (m_lBuySz+m_lSellSz);
    if(ltt<0.000001)
        return -1;

    return   m_lBuySz*100 / ltt;
}


