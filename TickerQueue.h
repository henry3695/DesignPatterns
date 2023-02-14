#ifndef TICKERQUEUE_H
#define TICKERQUEUE_H

#define LEN_NAME 8
#define LEN_INSTID 16

#include <QtGlobal>
struct TickerData
{
    char    instId[LEN_INSTID];
    char    pName[LEN_NAME];
    qint64     tickTime;
    double  last;
    double  lastSz;
    bool    bSell;
    bool    bShowPer;
    TickerData* pNext;
};
class TickerQueue
{
private:
    int            m_nSize;
    double         m_lTotalSz;
    double         m_lBuySz;
    double         m_lSellSz;
    int            m_nMaxSpan;
    bool           m_bFree;
    TickerData*    m_pHeadQueue;
public:
    TickerQueue(int nMaxSpan,bool bFree);
    void        AddNode(TickerData* pTickerData);
    void        CheckNode();
    double      GetTotalSz();
    int         GetSize();
    qint64      GetTickCount();
    double      GetBuySz();
    double      GetSellSz();
    double      Percentage();
};

#endif // TICKERQUEUE_H
