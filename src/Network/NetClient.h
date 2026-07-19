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
    void sendMessage(QByteArray data) const;
    void sendMSG2Local(QByteArray data) const;
    static QByteArray ExtractCommand(const QByteArray data);
    static void ToLittleEndian(QByteArray *ba);

private:
    QSharedPointer<QTcpSocket> txSocket;
    QTcpSocket* localSocket;
    bool verackSent=false;
    bool versionReceived = false;
    bool verAckreceived = false;
    void startHandShake();
    QByteArray buildVersionMsg();
    QByteArray buildVersinMSG_Payload();
    void sendVerAck();
    void initiateoutSocket();
    void startConnLocal();
    //void CreateConnection(const QString IP);

    void ProccessInvMsg(const QByteArray invArray , const QByteArray check) const;
    void createGetData(const QByteArray countPlushash, const QByteArray check) const;
    QByteArray constructGetDataHeader(const QByteArray invData , const QByteArray check) const;
    QByteArray CreatePongMessage(const QByteArray thePing) const;
    const QByteArray MagicWord = "F9BEB4D9";
    static QString Peer_IP;
    static QString localNode;


signals:
};
