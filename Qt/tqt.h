#ifndef TQT_H_
#define TQT_H_

#include <QtGui>
#include <QtCore>

#define MYDEBUG
#ifdef MYDEBUG
#define MYLOG	qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<(quint64)QThread::currentThreadId()<<__FUNCTION__
#else
#define MY_STUFF
#define MYLOG /MY_STUFF/
#endif

class Thread : public QThread
{
    Q_OBJECT
private:
    QTimer*  m_pTimerGlobal;
    int number;
protected:
    void run();
public:
    bool bExit;
    Thread(QObject *parent=0);
    ~Thread();
signals:
    void UpdateSignal(int num);
public slots:
    void ResetSlot();
    void Update();

};

class Widget : public QWidget
{
    Q_OBJECT
private:
    QLabel *label;
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *resetButton;
    Thread *myThread;
    int number;
public:
    Widget(QWidget *parent = 0);
    ~Widget();
signals:
    void ResetSignal();
    public slots:
        void ClearSlot();
        void StartSlot();
        void StopSlot();
        void UpdateSlot(int num);
};

#endif
