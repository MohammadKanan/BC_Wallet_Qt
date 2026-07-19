#ifndef DATABASEMAIN_H
#define DATABASEMAIN_H
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QObject>
#include <openssl/rsa.h>

class DataBaseMain : public QObject
{
    Q_OBJECT
public:
    explicit DataBaseMain(QObject *parent = nullptr);
    bool StoreNewWallet(std::string ID , RSA* pubKey , RSA* PrivKey, float balanace);


private:
    bool CreateSQLLiteDB();
    bool CreateTables();
    bool CreateWalletsTable();
    bool CreateDNSSeedsTable();
    QSqlDatabase bc_DB;

signals:
};

#endif // DATABASEMAIN_H
