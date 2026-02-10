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
    std::string sha256_2(const std::string input) const;
    QByteArray globalMSG;

private:
    QTimer connTimer;
    QTimer globalTimer;

    QSharedPointer<QTcpSocket> BC_Socket;
    void startConnection();
    bool verackSent=false;
    bool versionReceived = false;
    bool verAckreceived = false;
    bool abortPeer=false;
    bool handShakeComplete=false;
    bool isINV = false;
    QByteArray MSG_checksum;
    const QByteArray MagicWord = "F9BEB4D9";
    QString Peer_IP;
    void CreateConnection();
    void ProccessSocket();
    void sendMessage(const QByteArray data) const;
    void sendPong(const QByteArray pingPL) const;
    void sendVersion();
    QByteArray CreateVersionPL();
    void sendVerAck();
    void sendGetData(const QByteArray inventory);
    void sendAgetData();

signals:
    void sendGlobalMSG(const QByteArray data) const;

public slots:
    void HandleGlobalMSG(QByteArray data);
    void handleGlobalTimer();

};
