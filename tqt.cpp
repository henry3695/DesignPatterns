//tqt.cpp
#include "tqt.h"
#include <assert.h>
#include <windows.h>

Thread::Thread(QObject *parent)
{
    number = 0;
    bExit=false;
    m_pTimerGlobal=NULL;
}

Thread::~Thread()
{

}

void Thread::Update()
{
    number++;
     emit UpdateSignal(number);

     MYLOG<<QString::fromLocal8Bit("[子线程] 发送数据")<<"number="<<number;
}

void Thread::run()
{
    /*bExit=false;
    MYLOG<<QString::fromLocal8Bit("子线程");
    while(!bExit)
    {
        emit UpdateSignal(number);
        number++;
        sleep(1);
    }
    MYLOG<<QString::fromLocal8Bit("结束线程");*/
    if(m_pTimerGlobal==NULL)
    {
        m_pTimerGlobal = new QTimer(this);
        connect(m_pTimerGlobal, SIGNAL(timeout()), this, SLOT(Update()),Qt::DirectConnection);
        m_pTimerGlobal->start(1000);
        MYLOG<<QString::fromLocal8Bit("[子线程]创建定时器");
    }
    else
    {
        m_pTimerGlobal->moveToThread(this);
        m_pTimerGlobal->start(1000);
    }

    MYLOG<<QString::fromLocal8Bit("[子线程]子线程");
    Thread::exec();

    m_pTimerGlobal->stop();
    //sleep(3);
    MYLOG<<QString::fromLocal8Bit("[子线程]结束线程");
}

void Thread::ResetSlot()
{
    number = 0;
    emit UpdateSignal(11);
}

Widget::Widget(QWidget *parent)
{
    startButton = new QPushButton("start");
    stopButton = new QPushButton("stop");
    resetButton = new QPushButton("reset");
    label = new QLabel("empty");
    myThread = new Thread;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(startButton);
    layout->addWidget(stopButton);
    layout->addWidget(resetButton);
    setLayout(layout);

    connect(stopButton, SIGNAL(clicked()), this, SLOT(StopSlot()));
    connect(startButton, SIGNAL(clicked()),this, SLOT(StartSlot()));
    connect(resetButton, SIGNAL(clicked()),this, SLOT(ClearSlot()));
    connect(myThread, SIGNAL(UpdateSignal(int)),this, SLOT(UpdateSlot(int)));
    connect(this, SIGNAL(ResetSignal()),myThread, SLOT(ResetSlot()));

    setWindowTitle("Thread Test");
    resize(200, 200);
    myThread->start();

}

Widget::~Widget()
{

}

void Widget::StartSlot()
{
    myThread->start();
}

void Widget::StopSlot()
{
    //myThread->bExit=true;
    MYLOG<<QString::fromLocal8Bit("[主线程]准备退出");
    myThread->quit();

    MYLOG<<QString::fromLocal8Bit("[主线程]正在退出，等5秒");
    myThread->wait();

    MYLOG<<QString::fromLocal8Bit("[主线程]退出了");
}

void Widget::UpdateSlot(int num)
{
    label->setText(QString::number(num));
}

void Widget::ClearSlot()
{
    emit ResetSignal();
}
