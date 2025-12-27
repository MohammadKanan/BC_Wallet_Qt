#include "DataBaseMain.h"

#include <QDebug>

DataBaseMain::DataBaseMain(QObject *parent)
    : QObject{parent}
{
    if (!CreateSQLLiteDB())
        qDebug() << "Could not open DB";
    else
        qDebug() << "DB open correctly";


}

bool DataBaseMain::CreateSQLLiteDB()
{
    bc_DB = QSqlDatabase::addDatabase("QSQLITE");
    bc_DB.setDatabaseName("Kanan.bcDB");
    if(!bc_DB.open())
    {
        return false;

    }
    else
        bc_DB.close();
    return true;
}

bool DataBaseMain::CreateWalletsDB()
{
    if(!bc_DB.open())
        return false;
}
