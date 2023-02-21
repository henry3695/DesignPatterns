<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>Form</class>
 <widget class="QWidget" name="Form">
  <property name="geometry">
   <rect>
    <x>0</x>
    <y>0</y>
    <width>646</width>
    <height>330</height>
   </rect>
  </property>
  <property name="windowTitle">
   <string>Form</string>
  </property>
  <property name="layoutDirection">
   <enum>Qt::LeftToRight</enum>
  </property>
  <layout class="QVBoxLayout" name="verticalLayout">
   <property name="spacing">
    <number>6</number>
   </property>
   <property name="leftMargin">
    <number>4</number>
   </property>
   <property name="topMargin">
    <number>6</number>
   </property>
   <property name="rightMargin">
    <number>4</number>
   </property>
   <property name="bottomMargin">
    <number>6</number>
   </property>
   <item>
    <layout class="QHBoxLayout" name="horizontalLayout_8" stretch="0,0,0">
     <property name="spacing">
      <number>6</number>
     </property>
     <property name="sizeConstraint">
      <enum>QLayout::SetFixedSize</enum>
     </property>
     <item>
      <layout class="QHBoxLayout" name="horizontalLayout">
       <property name="sizeConstraint">
        <enum>QLayout::SetFixedSize</enum>
       </property>
       <property name="rightMargin">
        <number>20</number>
       </property>
       <item>
        <widget class="QLabel" name="label_symbol">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
         <property name="text">
          <string>symbol</string>
         </property>
        </widget>
       </item>
       <item>
        <widget class="QLineEdit" name="lineEdit_symbol">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
        </widget>
       </item>
      </layout>
     </item>
     <item>
      <layout class="QHBoxLayout" name="horizontalLayout_2">
       <property name="sizeConstraint">
        <enum>QLayout::SetFixedSize</enum>
       </property>
       <property name="rightMargin">
        <number>20</number>
       </property>
       <item>
        <widget class="QLabel" name="label_digits">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
         <property name="text">
          <string>digits</string>
         </property>
        </widget>
       </item>
       <item>
        <widget class="QLineEdit" name="lineEdit_digits">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
        </widget>
       </item>
      </layout>
     </item>
     <item>
      <layout class="QHBoxLayout" name="horizontalLayout_3">
       <property name="sizeConstraint">
        <enum>QLayout::SetFixedSize</enum>
       </property>
       <property name="rightMargin">
        <number>20</number>
       </property>
       <item>
        <widget class="QLabel" name="label_level1">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
         <property name="text">
          <string>level1</string>
         </property>
        </widget>
       </item>
       <item>
        <widget class="QLineEdit" name="lineEdit_level1">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
        </widget>
       </item>
      </layout>
     </item>
    </layout>
   </item>
   <item>
    <layout class="QHBoxLayout" name="horizontalLayout_9">
     <property name="spacing">
      <number>6</number>
     </property>
     <item>
      <layout class="QHBoxLayout" name="horizontalLayout_4">
       <property name="rightMargin">
        <number>20</number>
       </property>
       <property name="bottomMargin">
        <number>0</number>
       </property>
       <item>
        <widget class="QLabel" name="label_level2">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
         <property name="text">
          <string>level2</string>
         </property>
        </widget>
       </item>
       <item>
        <widget class="QLineEdit" name="lineEdit_level2">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
        </widget>
       </item>
      </layout>
     </item>
     <item>
      <layout class="QHBoxLayout" name="horizontalLayout_5">
       <property name="rightMargin">
        <number>20</number>
       </property>
       <item>
        <widget class="QLabel" name="label_level3">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
         <property name="text">
          <string>level3</string>
         </property>
        </widget>
       </item>
       <item>
        <widget class="QLineEdit" name="lineEdit_level3">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
        </widget>
       </item>
      </layout>
     </item>
     <item>
      <layout class="QHBoxLayout" name="horizontalLayout_6">
       <property name="rightMargin">
        <number>20</number>
       </property>
       <item>
        <widget class="QLabel" name="label_notify_level">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
         <property name="text">
          <string>notify_level</string>
         </property>
        </widget>
       </item>
       <item>
        <widget class="QLineEdit" name="lineEdit_notify_level">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
        </widget>
       </item>
      </layout>
     </item>
    </layout>
   </item>
   <item>
    <layout class="QHBoxLayout" name="horizontalLayout_7">
     <property name="rightMargin">
      <number>0</number>
     </property>
     <property name="bottomMargin">
      <number>0</number>
     </property>
     <item>
      <layout class="QHBoxLayout" name="horizontalLayout_10">
       <item>
        <widget class="QLabel" name="label_notify_ray">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
         <property name="layoutDirection">
          <enum>Qt::LeftToRight</enum>
         </property>
         <property name="text">
          <string>notify_ray</string>
         </property>
        </widget>
       </item>
       <item>
        <widget class="QLineEdit" name="lineEdit_notify_ray">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
        </widget>
       </item>
      </layout>
     </item>
     <item>
      <widget class="QPushButton" name="pushButtonAdd">
       <property name="text">
        <string>添加</string>
       </property>
      </widget>
     </item>
     <item>
      <widget class="QPushButton" name="pushButtonMod">
       <property name="text">
        <string>修改</string>
       </property>
      </widget>
     </item>
     <item>
      <widget class="QPushButton" name="pushButtonSave">
       <property name="text">
        <string>保存</string>
       </property>
      </widget>
     </item>
     <item>
      <widget class="QPushButton" name="pushButtonDel">
       <property name="text">
        <string>删除</string>
       </property>
      </widget>
     </item>
    </layout>
   </item>
   <item>
    <widget class="QListWidget" name="listWidget"/>
   </item>
  </layout>
 </widget>
 <resources/>
 <connections/>
</ui>


////////////////////
#ifndef FORM_H
#define FORM_H

#include <QWidget>
#include <QListWidgetItem>
namespace Ui {
class Form;
}

typedef struct _SymbolData
{
    QString symbol;
    qint32 digits;
    double level1;
    double level2;
    double level3;
    double notify_level;
    double notify_ray;
}SymbolData;
class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = 0);
    ~Form();

private slots:
    void on_listWidget_itemClicked(QListWidgetItem *item);

    void on_pushButtonAdd_clicked();

    void on_pushButtonSave_clicked();

    void on_pushButtonMod_clicked();

    void on_pushButtonDel_clicked();

private:
    Ui::Form *ui;


    bool ConvertToData(const QString& str,SymbolData& st);

    void setCtrlText(const SymbolData& st);
};

#endif // FORM_H

//////////////////////

#include "form.h"
#include "ui_form.h"
#include <qDebug>
#include <QStringList>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
Form::Form(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form)
{
    ui->setupUi(this);
    //ui->horizontalLayout->addStretch(2);
    //ui->horizontalLayout_2->addStretch(2);

    ui->horizontalLayout_7->addStretch(0);
    ui->horizontalLayout_8->addStretch(0);
    ui->horizontalLayout_9->addStretch(0);

    //ui->listWidget->addItem(QString("symbol:SPOT0,digits:5,level1:100,level2:2000,level3:3000,notify_level:5000,notify_ray:60"));
    //ui->listWidget->addItem(QString("symbol:SPOT1,digits:5,level1:100,level2:2000,level3:3000,notify_level:5000,notify_ray:60"));



    QFile f(qApp->applicationDirPath()+"\\symbol.txt");
    if(!f.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QMessageBox::information(this,"err","symbol.txt can not open",QMessageBox::Ok);
        return;
    }

    QString strLine;
    QTextStream txtInput(&f);
    while(!txtInput.atEnd())
    {
        strLine=txtInput.readLine();
        ui->listWidget->addItem(strLine);
    }

    f.close();
    //QMessageBox::information(this,"ok","symbol.txt save succ!",QMessageBox::Ok);
}

Form::~Form()
{
    delete ui;
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
    QString str = QString("symbol:%1,digits:5,level1:100,level2:2000,level3:3000,notify_level:5000,notify_ray:60")
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
    QFile f(qApp->applicationDirPath()+"\\symbol.txt");
    if(!f.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        QMessageBox::information(this,"err","symbol.txt can not open",QMessageBox::Ok);
        return;
    }

    QTextStream txtOutput(&f);

    for(int i=0;i<ui->listWidget->count();i++)
    {
        QString strItem = ui->listWidget->item(i)->text();
        txtOutput<<strItem<<endl;
    }
    f.close();
    QMessageBox::information(this,"ok","symbol.txt save succ!",QMessageBox::Ok);
}

void Form::on_pushButtonMod_clicked()
{
    if(ui->listWidget->currentItem()!=NULL)
    {
        QString str = QString("symbol:%1,digits:5,level1:100,level2:2000,level3:3000,notify_level:5000,notify_ray:60")
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
        QMessageBox::information(this,"err",QString::fromLocal8Bit("请先选择"),QMessageBox::Ok);
    }
}

void Form::on_pushButtonDel_clicked()
{
    QListWidgetItem* item = ui->listWidget->takeItem(ui->listWidget->currentRow());
    delete item;
}

