简单工厂模式
Abstract_Factory

工厂模式与策略模式
Factory_and_Strategy

工厂模式
Factory_Method


排序算法
ArraySorting

edit by henry

https://www.cnblogs.com/schips/p/12537360.html
    QDateTime datetime=QDateTime::currentDateTime();
    QString sTime = datetime.toString("yyyy-MM-dd hh:mm:ss");
   QString str = QString("[0x%1] [%2] void Widget::on_addItemBtn_clicked() ").arg((quint64)QThread::currentThreadId(),2,16, QLatin1Char('0')).arg(sTime);
   ui->listWidget->addItem(str);
   qDebug()<<(quint64)QThread::currentThreadId();
