
////////////////////
#ifndef FORM_H
#define FORM_H
#include "define.h"
#include <QWidget>
#include <QListWidgetItem>
#include "define.h"
#include <QNetworkReply>

typedef struct _stFenXi
{
	QString symbol;
	double  dlh1OIChangePercent;
	double  dlh4OIChangePercent;
	double  dlpriceChangePercent;
	double  dlvolUsd;
}stFenXi;

namespace Ui {
class Form;
}


class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = 0);
    ~Form();

signals:
    void signal_SymbolData(const SymbolData&);
	void signal_Message(const QString&);

private slots:
    void on_listWidget_itemClicked(QListWidgetItem *item);

    void on_pushButtonAdd_clicked();

    void on_pushButtonSave_clicked();

    void on_pushButtonMod_clicked();

    void on_pushButtonDel_clicked();

    void on_pushButtonOK_clicked();

    void on_pushButtonFX_clicked();

	void requestFinished(QNetworkReply* reply);

private:
    Ui::Form *ui;
    int m_IdSel;
	QMap<QString, QSet<QString>> m_qMapSwap;
	vector<stFenXi>  m_vecFenXi;

    bool ConvertToData(const QString& str,SymbolData& st);

    void setCtrlText(const SymbolData& st);

    void LoadData();

	void LoadSwapData();

    bool event(QEvent *event);

	void PostMessage(const QString& sMsg);


};

#endif // FORM_H
