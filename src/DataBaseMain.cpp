#include "DataBaseMain.h"
#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>
#include <QDebug>

DataBaseMain::DataBaseMain(QObject *parent)
    : QObject{parent}
{
    if (!CreateSQLLiteDB())
        qDebug() << "Could not open DB";
    else
        qDebug() << "DB open correctly";
    if (!CreateTables())
        qDebug() << "Error in tables creation!";


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

bool DataBaseMain::CreateTables()
{
    return (CreateWalletsTable());

}

bool DataBaseMain::CreateWalletsTable()
{
    if(!bc_DB.open())
        return false;
    QSqlQuery query(bc_DB);
    QString sqlStatement;

    //QMessageBox msgBox;
    sqlStatement = "CREATE TABLE IF NOT EXISTS ";
    sqlStatement += "Wallets ";
    sqlStatement += " (id INTEGER PRIMARY KEY NOT NULL, "
                    "publicKey varchar(50) NOT NULL, "
                    "privateKey varchar(100), "
                    "balance decimal(6,2) , "
                    "activeStatus INTEGER CHECK (activeStatus BETWEEN 0 AND 1));";
    //qDebug() << "Statement: " << sqlStatement;


    if (!query.exec(sqlStatement))
    {
        QString errorCode = query.lastError().text();
        qDebug() << errorCode;
        return false;
    }
    return true;
}

bool DataBaseMain::StoreNewWallet()
{
    return true;
}
