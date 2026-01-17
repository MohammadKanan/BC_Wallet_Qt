#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QTcpSocket>
#include <QDateTime>

class NetClient : public QObject
{
    Q_OBJECT
public:
    explicit NetClient(QObject *parent = nullptr);
    void sendMessage(QByteArray data);

private:
    QSharedPointer<QTcpSocket> txSocket;
    bool verackSent=false;
    void startHandShake();
    QByteArray buildVersionMsg();
    QByteArray buildVersinMSG_Payload();
    void ToLittleEndian(QByteArray *ba);
    void initiateoutSocket();
    QByteArray ExtractCommand(const QByteArray data) const;
    void ProccessInvMsg(const QByteArray invArray) const;


signals:
};
