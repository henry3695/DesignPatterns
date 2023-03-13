#include "CDataManagement.h"
#include "define.h"
CDataManagement::CDataManagement()
{

}

bool CDataManagement::OpenDateBase(const QString& sDbName)
{
	QString sDb = QString("%1/DB/%2").arg(qApp->applicationDirPath(), sDbName);
    mdbConn = QSqlDatabase::addDatabase("QSQLITE");
    mdbConn.setDatabaseName(sDb);

    // 打开数据库
    if (!mdbConn.open()) {
        qDebug() << "Error: Failed to open database.";
        return false;
    }
    return  true;
}
bool CDataManagement::Check5MTable()
{
	QString sTable = QString("CREATE TABLE  IF NOT EXISTS tb_5mSymbol ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT, "
		"symbol TEXT NOT NULL, "
		"b TEXT NOT NULL, "
		"s TEXT NOT NULL,"
		"tick INTEGER NOT NULL,"
		"t TEXT NOT NULL)");
	QSqlQuery query;
	query.prepare(sTable);
	if (!query.exec()) {
		qDebug() << "Error: Failed to create table.";
		return false;
	}
	return true;
}
bool  CDataManagement::Add5MData(const QString& symbol, quint64 buy, quint64 sell)
{	
	QSqlQuery query;
	// 添加数据
	query.prepare("INSERT INTO tb_5mSymbol (symbol, b, s,tick,t) "
		"VALUES (:symbol, :b, :s,:tick,:t)");
	query.bindValue(":symbol", symbol);
	query.bindValue(":b", QString::number(buy));
	query.bindValue(":s", QString::number(sell));
	query.bindValue(":tick", QDateTime::currentDateTime().toMSecsSinceEpoch()/1000);
	query.bindValue(":t", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

	if (!query.exec()) {
		qDebug() << "Error: Failed to insert data.";
		return false;
	}

	return true;
}

bool CDataManagement::Close()
{
	mdbConn.close();
	return true;
}

void CDataManagement::DoTest()
{
	OpenDateBase("test.DB");
    // 创建表格
    QSqlQuery query;
    query.prepare("CREATE TABLE  IF NOT EXISTS student_info ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "name TEXT NOT NULL, "
                  "age INTEGER NOT NULL, "
                  "gender TEXT NOT NULL)");

    if (!query.exec()) {
        qDebug() << "Error: Failed to create table.";
        return;
    }

    // 添加数据
    query.prepare("INSERT INTO student_info (name, age, gender) "
                  "VALUES (:name, :age, :gender)");
    query.bindValue(":name", TEXT_CU("张三"));
    query.bindValue(":age", qint64(-0x7FFFFFFF));
    query.bindValue(":gender", TEXT_CU("男"));

    if (!query.exec()) {
        qDebug() << "Error: Failed to insert data.";
        return;
    }

    // 修改数据
    query.prepare("UPDATE student_info SET age=:age WHERE name=:name");
    query.bindValue(":age", qint64(0x7FFFFFFF));
    query.bindValue(":name", TEXT_CU("张三"));

    if (!query.exec()) {
        qDebug() << "Error: Failed to update data.";
        return;
    }
#if 0
    // 删除数据
    query.prepare("DELETE FROM student_info WHERE name=:name");
    query.bindValue(":name", TEXT_CU("张三"));

    if (!query.exec()) {
        qDebug() << "Error: Failed to delete data.";
        return;
    }
#endif
    // 查询数据
    query.prepare("SELECT * FROM student_info");
    if (!query.exec()) {
        qDebug() << "Error: Failed to query data.";
        return;
    }

    while (query.next()) {
        QString name = query.value("name").toString();
        int age = query.value("age").toUInt();
        qDebug()<<age<<name;
    }
}
