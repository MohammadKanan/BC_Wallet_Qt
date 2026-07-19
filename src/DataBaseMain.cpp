#include "DataBaseMain.h"
#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>
#include <QDebug>
#include <openssl/pem.h>
QByteArray rsaPublicKeyToByteArrayPem(RSA* rsaKey) {
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) return QByteArray();

    // Use the appropriate OpenSSL function
    if (PEM_write_bio_RSAPublicKey(bio, rsaKey) <= 0) {
        BIO_free_all(bio);
        return QByteArray();
    }

    size_t keyLen = BIO_pending(bio);
    QByteArray byteArray(keyLen, '\0');
    BIO_read(bio, byteArray.data(), keyLen);

    BIO_free_all(bio);
    byteArray = "'" + byteArray;
    byteArray += "'";
    return byteArray;
}
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
                    "WalletName  varChar(20) ,"
                    "publicKey varchar(500) NOT NULL, "
                    "privateKey varchar(500), "
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

bool DataBaseMain::StoreNewWallet(std::string ID , RSA* pubKey , RSA* PrivKey, float balanace)
{
    if(!bc_DB.open())
        return false;
    QSqlQuery query(bc_DB);
    QString sqlStatement;
    sqlStatement = "INSERT INTO Wallets (WalletName , publicKey , privateKey , balance , activeStatus) VALUES ('";
    sqlStatement += ID;
    sqlStatement += "'";
    sqlStatement += ",";
    sqlStatement += QString::fromUtf8(rsaPublicKeyToByteArrayPem(pubKey));
    sqlStatement += ",";
    sqlStatement += QString::fromUtf8(rsaPublicKeyToByteArrayPem(PrivKey)) ;
    sqlStatement += ",";
    sqlStatement += QString::number(balanace) ;
    sqlStatement += ",";
    sqlStatement += "1";
    sqlStatement += ");";
    //qDebug() << "Insert statement :" << sqlStatement;
    if(!query.exec(sqlStatement)){
        QString errorCode = query.lastError().text();
        qDebug() << errorCode;
        return false;
    }
    return true;
}
