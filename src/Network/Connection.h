#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QTcpSocket>
#include <QDateTime>
#include <QTimer>

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(QString PeerIPAddress, QObject *parent = nullptr);
    std::string sha256_2(const std::string input);

private:
    QTimer connTimer;
    QSharedPointer<QTcpSocket> BC_Socket;
    void startConnection();
    bool verackSent=false;
    bool versionReceived = false;
    bool verAckreceived = false;
    bool isINV = false;
    QByteArray MSG_checksum;
    const QByteArray MagicWord = "F9BEB4D9";
    QString Peer_IP;
    void CreateConnection();
    void ProccessSocket();
    void sendMessage(QByteArray data) const;
    void sendPong(const QByteArray pingPL) const;
    void sendVersion();
    QByteArray CreateVersionPL();
    void sendVerAck();
    void sendGetData(const QByteArray inventory);



signals:
};
