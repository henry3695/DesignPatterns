#ifndef CDATAMANAGEMENT_H
#define CDATAMANAGEMENT_H
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

class CDataManagement
{
public:
    CDataManagement();


    bool OpenDateBase(const QString& sDbName);
	bool Check5MTable();
	bool Add5MData(const QString& symbol,quint64 buy, quint64 sell);
	bool Close();
	
	void DoTest();

private:
     QSqlDatabase mdbConn;
};

#endif // CDATAMANAGEMENT_H
