#include <QCoreApplication>
#include <QtCore>
#include <QtGlobal>
#define MYDEBUG
#ifdef MYDEBUG
#define MYLOG	qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<QThread::currentThread()<<(quint64)QThread::currentThreadId()<<__FUNCTION__
#else
#define MY_STUFF
#define MYLOG /MY_STUFF/
#endif
quint8 convert_data(quint8 ch1,quint8 ch2)
{
    if(ch1>='a' && ch1<='f')
    {
        ch1=(ch1-'a')+10;
    }
    else
    {
        ch1=(ch1-'0');
    }

    if(ch2>='a' && ch2<='f')
    {
        ch2=(ch2-'a')+10;
    }
    else
    {
        ch2=(ch2-'0');
    }

    return quint8(  (ch1<<4)|ch2 );
}
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    MYLOG<<qApp->applicationDirPath()+"/test.rar";
#if 0
    QByteArray mData;
    QFile f(qApp->applicationDirPath()+"/test.rar");
    if(f.open(QIODevice::ReadOnly))
    {
        mData = f.readAll();
        f.close();
        MYLOG<<mData.count();

        QString hex;
        for(int i=0;i<mData.count();i++)
        {
            quint8 qCh = mData.at(i);
            hex += QString("%1").arg(qCh, 2, 16, QLatin1Char('0'));  // 保留四位，不足补零
        }
        qDebug()<<hex;

         QFile ftxt(qApp->applicationDirPath()+"/test.txt");
         if(ftxt.open(QIODevice::WriteOnly|QIODevice::Text))
         {
             mData=hex.toUtf8();
             ftxt.write(mData);
         }
         ftxt.close();
    }
#else
    QByteArray mData;
    QFile f(qApp->applicationDirPath()+"/test.txt");
    if(f.open(QIODevice::ReadOnly))
    {
        mData = f.readAll();
        f.close();
        MYLOG<<mData.count();

        QByteArray mData1;
        mData1.resize(mData.count()/2);
        for(int i=0;i<mData.count();i+=2)
        {
            quint8 qCh1 = mData.at(i);
            quint8 qCh2 = mData.at(i+1);
            mData1[i/2] = convert_data(qCh1,qCh2);
        }

         QFile ftxt(qApp->applicationDirPath()+"/test.txt.rar");
         if(ftxt.open(QIODevice::WriteOnly))
         {
             ftxt.write(mData1);
         }
         ftxt.close();
    }

#endif
    return a.exec();
}
