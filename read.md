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

https://www.cnblogs.com/schips/p/12537360.html

Qt多线程通信
https://blog.csdn.net/hebbely/article/details/79428336

C++ WebSocket网络通信应用
https://blog.csdn.net/wwplh5520370/article/details/115343065

json转c++结构体
https://github.com/xyz347/xpack

c++ 类的默认八种函数
https://blog.csdn.net/JMW1407/article/details/108785842

MobaXterm.exe
HeidiSQL
FSCapture.exe
doxygen
亿图图示

https://github.com/alibabacloud-go/alidns-20150109/tree/master/client


qt 列子
https://doc.qt.io/qt-5/qtwebsockets-examples.html


<%
' 存储数组对象
Dim yourArray(2)
yourArray(0) = "Item 1"
yourArray(1) = "Item 2"
yourArray(2) = "Item 3"

Application("MyArray") = yourArray
%>

<%
' 检索数组对象
Dim retrievedArray
retrievedArray = Application("MyArray")

' 使用数组对象
For i = LBound(retrievedArray) To UBound(retrievedArray)
    Response.Write(retrievedArray(i) & "<br>")
Next
%>




