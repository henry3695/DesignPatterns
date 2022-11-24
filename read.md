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

  QStandardItemModel* model = new QStandardItemModel();
    //QStringList labels = QObject::trUtf8("频率,功率,误差").simplified().split(",");
    //model->setHorizontalHeaderLabels(labels);

    //定义item
    QStandardItem* item = 0;
    for(int i = 0;i < 10;i++){
        item = new QStandardItem(QString("10000%1").arg(i));
        model->setItem(i,0,item);
        item = new QStandardItem(QString("10000%1").arg(i*2));
        model->setItem(i,1,item);
        item = new QStandardItem(QString("10000%1").arg(i*3));
        model->setItem(i,2,item);
    }
    ui->tableView->setModel(model);
    item = new QStandardItem(QString("11111%1").arg(3));
    model->setItem(9,0,item);
     item = new QStandardItem(QString("222222%1").arg(3));
    model->setItem(9,1,item);

    ui->tableView->horizontalHeader()->hide();
    ui->tableView->verticalHeader()->hide();
    ui->tableView->setGridStyle(Qt::NoPen);
     //ui->tableView->setGridStyle(Qt::PenStyle::NoPen);
    ui->tableView->setFrameStyle(QFrame::NoFrame);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //ui->tableView->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);


    ui->tableView->verticalHeader()->setDefaultSectionSize(20);
    ui->tableView->horizontalHeader()->setDefaultSectionSize(60);

    //ui->tableView->verticalHeader()->setDefaultSectionSize(100);
    //ui->tableView->horizontalHeader()->setDefaultSectionSize(100);

    ui->tableView->resize(220,230);

    ui->tableView->show();
    ui->tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff );
    ui->tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff );
