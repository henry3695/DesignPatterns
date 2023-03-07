

//////////////////////

#include "form.h"
#include "ui_form.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#define SYMBOL_PATH "\\Config\\symbol.txt"




Form::Form(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form),
    m_IdSel(0)
{
    ui->setupUi(this);
    //ui->horizontalLayout->addStretch(2);
    //ui->horizontalLayout_2->addStretch(2);

    ui->horizontalLayout_7->addStretch(0);
    ui->horizontalLayout_8->addStretch(0);
    ui->horizontalLayout_9->addStretch(0);

    //ui->listWidget->addItem(QString("symbol:SPOT0,digits:5,level1:100,level2:2000,level3:3000,notify_level:5000,notify_ray:60"));
    //ui->listWidget->addItem(QString("symbol:SPOT1,digits:5,level1:100,level2:2000,level3:3000,notify_level:5000,notify_ray:60"));
    LoadData();

	LoadSwapData();
}

Form::~Form()
{
    delete ui;
}
void Form::LoadData()
{
    ui->listWidget->clear();
    QFile f(qApp->applicationDirPath()+SYMBOL_PATH);
    if(!f.open(QIODevice::ReadOnly|QIODevice::Text))
    {
		PostMessage(TEXT_CU("找不到 symbol.txt 文件!"));
        return;
    }

    QString strLine;
    QString selLine;
    QTextStream txtInput(&f);
    int i = 0;
    while(!txtInput.atEnd())
    {
        strLine=txtInput.readLine();
        ui->listWidget->addItem(strLine);
        if(i==m_IdSel)
        {
            selLine=strLine;
        }
    }

    f.close();

    SymbolData st;
    if(Form::ConvertToData(selLine,st))
    {
        g_SymbolData=st;
    }
    ui->listWidget->setCurrentRow(m_IdSel);

    QString sMsg=QString("m_IdSel=%1").arg(m_IdSel);
    this->setWindowTitle(sMsg);
}

void Form::LoadSwapData()
{
	m_qMapSwap.clear();
	m_qMapSwap["okx"] = QSet<QString>{ "BTCUSDT" };
	m_qMapSwap["bybit"] = QSet<QString>{ "BTCUSDT" };
	m_qMapSwap["ban"] = QSet<QString>{ "BTCUSDT" };

    qDebug()<<qApp->applicationDirPath();
	//https://api.bybit.com/v2/public/symbols
	QFile f(qApp->applicationDirPath() + "\\Config\\bybit_swap.json");
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::information(this, "err", TEXT_CU("找不到 bybit_swap.json 文件!"), QMessageBox::Ok);
		return;
	}

	QString sTxt = f.readAll();
	f.close();

	QJsonParseError err_rpt;
	QJsonDocument  root_Doc = QJsonDocument::fromJson(sTxt.toUtf8(), &err_rpt); // 字符串格式化为JSON

	if (err_rpt.error != QJsonParseError::NoError)
	{		
		QMessageBox::information(this, "err", TEXT_CU("bybit_swap.json 文件解析出错!"), QMessageBox::Ok);
		return;
	}
	else
	{
		QJsonObject root_Obj = root_Doc.object();
		QJsonValue ret_code = root_Obj.value("ret_code");
		QJsonValue result = root_Obj.value("result");
		if (!ret_code.isNull() && ret_code.toInt() == 0)
		{		
			if (result.isArray())
			{
				QJsonArray array = result.toArray();
				for (int i = 0; i < array.size(); i++)
				{
					QJsonValue iconArray = array.at(i);
					QJsonObject node = iconArray.toObject();
					QJsonValue val = node.value("name");
					//MYLOG << val.toString();					
					m_qMapSwap["bybit"].insert(val.toString().trimmed());
				}
			}
			PostMessage(TEXT_CU("bybit_swap.json 文件解析成功！"));
		}	
		else
		{
			QMessageBox::information(this, "err", TEXT_CU("bybit_swap.json 文件解析出错!"), QMessageBox::Ok);
			return;
		}
	}

	//https://www.okx.com/api/v5/public/instruments?instType=SWAP
	f.setFileName(qApp->applicationDirPath() + "\\Config\\okx_swap.json");
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::information(this, "err", TEXT_CU("找不到 okx_swap.json 文件!"), QMessageBox::Ok);
		return;
	}


	sTxt = f.readAll();
	f.close();

	root_Doc = QJsonDocument::fromJson(sTxt.toUtf8(), &err_rpt); // 字符串格式化为JSON
	if (err_rpt.error != QJsonParseError::NoError)
	{
		QMessageBox::information(this, "err", TEXT_CU("okx_swap.json 文件解析出错!"), QMessageBox::Ok);
		return;
	}
	else
	{
		QJsonObject root_Obj = root_Doc.object();
		QJsonValue ret_code = root_Obj.value("code");
		QJsonValue result = root_Obj.value("data");
		if (!ret_code.isNull() && ret_code.toInt() == 0)
		{
			if (result.isArray())
			{
				QJsonArray array = result.toArray();
				for (int i = 0; i < array.size(); i++)
				{
					QJsonValue iconArray = array.at(i);
					QJsonObject node = iconArray.toObject();
					QJsonValue val = node.value("instId");
					//MYLOG << val.toString();					
					m_qMapSwap["okx"].insert(val.toString().trimmed());
				}
			}
			PostMessage(TEXT_CU("okx_swap.json 文件解析成功！"));
		}
		else
		{
			QMessageBox::information(this, "err", TEXT_CU("okx_swap.json 文件解析出错!"), QMessageBox::Ok);
			return;
		}
	}

	//https://fapi.binance.com/fapi/v1/exchangeInfo
	f.setFileName(qApp->applicationDirPath() + "\\Config\\ban_swap.json");
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::information(this, "err", TEXT_CU("找不到 ban_swap.json 文件!"), QMessageBox::Ok);
		return;
	}


	sTxt = f.readAll();
	f.close();

	root_Doc = QJsonDocument::fromJson(sTxt.toUtf8(), &err_rpt); // 字符串格式化为JSON
	if (err_rpt.error != QJsonParseError::NoError)
	{
		QMessageBox::information(this, "err", TEXT_CU("ban_swap.json 文件解析出错!"), QMessageBox::Ok);
		return;
	}
	else
	{
		QJsonObject root_Obj = root_Doc.object();
		QJsonValue result = root_Obj.value("symbols");
		
		if (!result.isNull() && result.isArray())
		{
			QJsonArray array = result.toArray();
			for (int i = 0; i < array.size(); i++)
			{
				QJsonValue iconArray = array.at(i);
				QJsonObject node = iconArray.toObject();
				QJsonValue val = node.value("symbol");
				//MYLOG << val.toString();					
				m_qMapSwap["ban"].insert(val.toString().trimmed());
			}
		}
		PostMessage(TEXT_CU("ban_swap.json 文件解析成功！"));
	}
}
void Form::on_listWidget_itemClicked(QListWidgetItem *item)
{
    qDebug()<<item->text();

    SymbolData st;
    bool b = ConvertToData(item->text(),st);
    if(b)
    {
        setCtrlText(st);
    }
}

bool  Form::event(QEvent *event)
{
    if(event->type()==QEvent::Show)
    {
        MYLOG<<event->type();
        LoadData();
    }
    return  QWidget::event(event);
}

void Form::setCtrlText(const SymbolData& st)
{
    ui->lineEdit_symbol->setText(st.symbol);
    ui->lineEdit_digits->setText(QString::number(st.digits,10));
    ui->lineEdit_level1->setText(QString::number(st.level1,10,0));
    ui->lineEdit_level2->setText(QString::number(st.level2,10,0));
    ui->lineEdit_level3->setText(QString::number(st.level3,10,0));
    ui->lineEdit_notify_level->setText(QString::number(st.notify_level,10,0));
    ui->lineEdit_notify_ray->setText(QString::number(st.notify_ray,10,0));
}
bool  Form::ConvertToData(const QString& str,SymbolData& st)
{
    QStringList qList = str.split(",");
    for (int i=0;i<qList.count();i++)
    {
        QStringList ls = qList.at(i).split(":");
        if(ls.count()!=2)
        {
            qDebug()<<str;
            return false;
        }
        QString s0 = ls[0];
        QString s1 = ls[1];
        if(s0=="symbol")
        {
            st.symbol=s1;
        }
        else if(s0=="digits")
        {
            st.digits=s1.toInt();
        }
        else if(s0=="level1")
        {
            st.level1=s1.toDouble();
        }
        else if(s0=="level2")
        {
            st.level2=s1.toDouble();
        }
        else if(s0=="level3")
        {
            st.level3=s1.toDouble();
        }
        else if(s0=="notify_level")
        {
            st.notify_level=s1.toDouble();
        }
        else if(s0=="notify_ray")
        {
            st.notify_ray=s1.toDouble();
        }
    }
    return true;
}

void Form::on_pushButtonAdd_clicked()
{
    QString str = QString("symbol:%1,digits:%2,level1:%3,level2:%4,level3:%5,notify_level:%6,notify_ray:%7")
            .arg(ui->lineEdit_symbol->text())
            .arg(ui->lineEdit_digits->text())
            .arg(ui->lineEdit_level1->text())
            .arg(ui->lineEdit_level2->text())
            .arg(ui->lineEdit_level3->text())
            .arg(ui->lineEdit_notify_level->text())
            .arg(ui->lineEdit_notify_ray->text());

    ui->listWidget->addItem(str);
}

void Form::on_pushButtonSave_clicked()
{
    QFile f(qApp->applicationDirPath()+SYMBOL_PATH);
    if(!f.open(QIODevice::WriteOnly|QIODevice::Text))
    {
		PostMessage("symbol.txt can not open");
        return;
    }

    QTextStream txtOutput(&f);

    for(int i=0;i<ui->listWidget->count();i++)
    {
        QString strItem = ui->listWidget->item(i)->text();
        txtOutput<<strItem<<endl;
    }
    f.close();
	PostMessage(TEXT_CU("symbol.txt 保存成功!"));
}

void Form::on_pushButtonMod_clicked()
{
    if(ui->listWidget->currentItem()!=NULL)
    {
        QString str = QString("symbol:%1,digits:%2,level1:%3,level2:%4,level3:%5,notify_level:%6,notify_ray:%7")
                .arg(ui->lineEdit_symbol->text())
                .arg(ui->lineEdit_digits->text())
                .arg(ui->lineEdit_level1->text())
                .arg(ui->lineEdit_level2->text())
                .arg(ui->lineEdit_level3->text())
                .arg(ui->lineEdit_notify_level->text())
                .arg(ui->lineEdit_notify_ray->text());

        ui->listWidget->currentItem()->setText(str);
    }
    else
    {
		PostMessage(TEXT_CU("请先选择!"));
    }
}

void Form::on_pushButtonDel_clicked()
{
    if(ui->listWidget->currentItem()!=NULL)
    {
        QListWidgetItem* item = ui->listWidget->takeItem(ui->listWidget->currentRow());
        delete item;
    }
    else
    {
		PostMessage(TEXT_CU("请先选择!"));
    }
}


void Form::on_pushButtonOK_clicked()
{
    if(ui->listWidget->currentItem()!=NULL)
    {
        SymbolData st;
        if(Form::ConvertToData(ui->listWidget->currentItem()->text(),st))
        {
            m_IdSel=ui->listWidget->currentRow();
            emit signal_SymbolData(st);

			const QString& symbol = st.symbol;
			
			{
				const QSet<QString>& qSet = m_qMapSwap["ban"];
				QString swap_name = QString("%1USDT").arg(symbol);
				if (qSet.find(swap_name) != qSet.end())
				{
					PostMessage(swap_name + TEXT_CU(",ban succ！"));
				}
				else
				{
					PostMessage(swap_name + TEXT_CU(",ban error！"));
				}

				swap_name = QString("%1USD").arg(symbol);
				if (qSet.find(swap_name) != qSet.end())
				{
					PostMessage(swap_name + TEXT_CU(",ban succ！"));
				}
				else
				{
					PostMessage(swap_name + TEXT_CU(",ban error！"));
				}
			}

			{
				const QSet<QString>& qSet = m_qMapSwap["okx"];
				QString swap_name = QString("%1-USDT-SWAP").arg(symbol);
				if (qSet.find(swap_name) != qSet.end())
				{
					PostMessage(swap_name + TEXT_CU(",okx succ！"));
				}
				else
				{
					PostMessage(swap_name + TEXT_CU(",okx error！"));
				}

				swap_name = QString("%1-USD-SWAP").arg(symbol);
				if (qSet.find(swap_name) != qSet.end())
				{
					PostMessage(swap_name + TEXT_CU(",okx succ！"));
				}
				else
				{
					PostMessage(swap_name + TEXT_CU(",okx error！"));
				}
			}

			{
				const QSet<QString>& qSet = m_qMapSwap["bybit"];
				QString swap_name = QString("%1USDT").arg(symbol);
				if (qSet.find(swap_name) != qSet.end())
				{
					PostMessage(swap_name + TEXT_CU(",bybit succ！"));
				}
				else
				{
					PostMessage(swap_name + TEXT_CU(",bybit error！"));
				}

				swap_name = QString("%1USD").arg(symbol);
				if (qSet.find(swap_name) != qSet.end())
				{
					PostMessage(swap_name + TEXT_CU(",bybit succ！"));
				}
				else
				{
					PostMessage(swap_name + TEXT_CU(",bybit error！"));
				}
			}

        }
        else
        {
			PostMessage(TEXT_CU("数据格式不正确!"));
        }
    }
    else
    {
		PostMessage(TEXT_CU("请先选择!"));
    }
}

void Form::PostMessage(const QString& sMsg)
{
	QString sMsg1 = QString("[%1] %2").arg((quint64)QThread::currentThreadId()).arg(sMsg);
	emit signal_Message(sMsg1);
}
void Form::on_pushButtonFX_clicked()
{
#if 1
	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFinished(QNetworkReply*)));
	Q_ASSERT(connRet);

	QString sUrl = QString("https://fapi.coinglass.com/api/futures/coins/markets");
	request.setUrl(QUrl(sUrl));
	QNetworkReply* reply = naManager->get(request);

#else
	QString sPath(qApp->applicationDirPath() + "\\Config\\top100.json");
    QFile f;
    f.setFileName(sPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::information(this, "err", TEXT_CU("找不到 top100.json 文件!"), QMessageBox::Ok);
        return;
    }

    QString sTxt = f.readAll();
    f.close();

    QJsonParseError err_rpt;
    QJsonDocument  root_Doc;
    root_Doc = QJsonDocument::fromJson(sTxt.toUtf8(), &err_rpt); // 字符串格式化为JSON
    if (err_rpt.error != QJsonParseError::NoError)
    {
        QMessageBox::information(this, "err", TEXT_CU("top100.json 文件解析出错!"), QMessageBox::Ok);
        return;
    }
    else
    {
        QJsonObject root_Obj = root_Doc.object();
        QJsonValue code = root_Obj.value("code");
        if(!code.isNull() && code.toString()=="0")
        {
            QJsonValue result = root_Obj.value("data");

            if (!result.isNull() && result.isArray())
            {
				m_vecFenXi.clear();
                QJsonArray array = result.toArray();
                for (int i = 0; i < array.size(); i++)
                {
                    QJsonValue iconArray = array.at(i);
                    QJsonObject node = iconArray.toObject();
                    QJsonValue symbol = node.value("symbol");
                    QJsonValue h1OIChangePercent = node.value("h1OIChangePercent");
                    QJsonValue h4OIChangePercent = node.value("h4OIChangePercent");
                    QJsonValue priceChangePercent = node.value("priceChangePercent");
                    QJsonValue volUsd = node.value("volUsd");

					/*
					MYLOG << symbol.toString()<<h1OIChangePercent.toDouble()<<
                             h4OIChangePercent.toDouble()<<priceChangePercent.toDouble()<<
                             volUsd.toDouble();
`					*/

					stFenXi st;
					st.symbol = symbol.toString();
					st.dlh1OIChangePercent = h1OIChangePercent.toDouble();
					st.dlh4OIChangePercent = h4OIChangePercent.toDouble();
					st.dlpriceChangePercent = priceChangePercent.toDouble();
					st.dlvolUsd = volUsd.toDouble();
					m_vecFenXi.push_back(st);
                }
                PostMessage(TEXT_CU("top100.json 文件解析成功！"));


				QString sMsg;
				sort(m_vecFenXi.begin(), m_vecFenXi.end(), 
					[](const stFenXi& a,const stFenXi& b)->bool 
					{
						return a.dlvolUsd > b.dlvolUsd; 
					}
				);

				vector<stFenXi> vec;
				for (int i=0;i<30;i++)
				{
					const stFenXi& st = m_vecFenXi[i];
					/*sMsg = QString("%1,%2,%3,%4,%5").arg(st.symbol).arg(st.dlpriceChangePercent)
						.arg(st.dlh4OIChangePercent).arg(st.dlh1OIChangePercent).arg(st.dlvolUsd);*/

					//PostMessage(sMsg);					
					vec.push_back(st);
				}
				sort(vec.begin(), vec.end(),
					[](const stFenXi& a, const stFenXi& b)->bool
				{
					return fabs(a.dlh4OIChangePercent) > fabs(b.dlh4OIChangePercent);
				}
				);

				for (int i = 0; i < vec.size(); i++)
				{
					const stFenXi& st = vec[i];
					sMsg = QString("%1,%2,%3,%4,%5").arg(st.symbol).arg(st.dlpriceChangePercent)
						.arg(st.dlh4OIChangePercent).arg(st.dlh1OIChangePercent).arg(st.dlvolUsd);

					PostMessage(sMsg);	
				}

                return;
            }
        }

        QMessageBox::information(this, "err", TEXT_CU("top100.json 文件解析出错!"), QMessageBox::Ok);
        return;
    }

#endif
}



void Form::requestFinished(QNetworkReply* reply)
{
	// 获取http状态码
	QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if (statusCode.isValid())
		qDebug() << "status code=" << statusCode.toInt();

	QVariant reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
	if (reason.isValid())
		qDebug() << "reason=" << reason.toString();

	QNetworkReply::NetworkError err = reply->error();
	if (err != QNetworkReply::NoError) {
		qDebug() << "Failed: " << reply->errorString();
	}
	else {
		// 获取返回内容
		string str = reply->readAll().toStdString();
		QString sTxt = str.c_str();

		QJsonParseError err_rpt;
		QJsonDocument  root_Doc;
		root_Doc = QJsonDocument::fromJson(sTxt.toUtf8(), &err_rpt); // 字符串格式化为JSON
		if (err_rpt.error != QJsonParseError::NoError)
		{
			QMessageBox::information(this, "err", TEXT_CU("top100.json 文件解析出错!"), QMessageBox::Ok);
			return;
		}
		else
		{
			QJsonObject root_Obj = root_Doc.object();
			QJsonValue code = root_Obj.value("code");
			if (!code.isNull() && code.toString() == "0")
			{
				QJsonValue result = root_Obj.value("data");

				if (!result.isNull() && result.isArray())
				{
					m_vecFenXi.clear();
					QJsonArray array = result.toArray();
					for (int i = 0; i < array.size(); i++)
					{
						QJsonValue iconArray = array.at(i);
						QJsonObject node = iconArray.toObject();
						QJsonValue symbol = node.value("symbol");
						QJsonValue h1OIChangePercent = node.value("h1OIChangePercent");
						QJsonValue h4OIChangePercent = node.value("h4OIChangePercent");
						QJsonValue priceChangePercent = node.value("priceChangePercent");
						QJsonValue volUsd = node.value("volUsd");

						/*
						MYLOG << symbol.toString()<<h1OIChangePercent.toDouble()<<
						h4OIChangePercent.toDouble()<<priceChangePercent.toDouble()<<
						volUsd.toDouble();
						`					*/

						stFenXi st;
						st.symbol = symbol.toString();
						st.dlh1OIChangePercent = h1OIChangePercent.toDouble();
						st.dlh4OIChangePercent = h4OIChangePercent.toDouble();
						st.dlpriceChangePercent = priceChangePercent.toDouble();
						st.dlvolUsd = volUsd.toDouble();
						m_vecFenXi.push_back(st);
					}
					PostMessage(TEXT_CU("top100.json 文件解析成功！"));


					QString sMsg;
					sort(m_vecFenXi.begin(), m_vecFenXi.end(),
						[](const stFenXi& a, const stFenXi& b)->bool
					{
						return a.dlvolUsd > b.dlvolUsd;
					}
					);

					vector<stFenXi> vec;
					for (int i = 0; i<30; i++)
					{
						const stFenXi& st = m_vecFenXi[i];
						/*sMsg = QString("%1,%2,%3,%4,%5").arg(st.symbol).arg(st.dlpriceChangePercent)
						.arg(st.dlh4OIChangePercent).arg(st.dlh1OIChangePercent).arg(st.dlvolUsd);*/

						//PostMessage(sMsg);					
						vec.push_back(st);
					}
					sort(vec.begin(), vec.end(),
						[](const stFenXi& a, const stFenXi& b)->bool
					{
						return fabs(a.dlh1OIChangePercent) > fabs(b.dlh1OIChangePercent);
					}
					);

					for (int i = 0; i < 10; i++)

					{
						const stFenXi& st = vec[i];
						sMsg = QString("%1,%2,%3,%4,%5").arg(st.symbol).arg(st.dlpriceChangePercent)
							.arg(st.dlh4OIChangePercent).arg(st.dlh1OIChangePercent).arg(st.dlvolUsd);

						PostMessage(sMsg);
					}

					return;
				}
			}

			QMessageBox::information(this, "err", TEXT_CU("top100.json 文件解析出错!"), QMessageBox::Ok);
			return;
		}
		
	}
}