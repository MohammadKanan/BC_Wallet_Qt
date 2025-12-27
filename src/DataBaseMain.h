#ifndef DATABASEMAIN_H
#define DATABASEMAIN_H
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QObject>

class DataBaseMain : public QObject
{
    Q_OBJECT
public:
    explicit DataBaseMain(QObject *parent = nullptr);

private:
    bool CreateSQLLiteDB();
    bool CreateTables();
    bool CreateWalletsTable();
    bool StoreNewWallet();
    QSqlDatabase bc_DB;

signals:
};

#endif // DATABASEMAIN_H
