#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QTcpSocket>
#include <QDateTime>
#include <QTimer>
#include "RawMessage.h"

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(QString PeerIPAddress, QObject *parent = nullptr);
    std::string sha256_2(const std::string input) const;
    QByteArray globalMSG;
    static void ToLittleEndian(QByteArray *ba);

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
    void ReadSocket();
    int GetPayloadSizeFromHeader(const QByteArray data);
    void SplitMultipleCommands(const QByteArray data);
    void ProccessIncomingCommand(const QByteArray data);
    void sendMessage(const QByteArray data) const;
    void sendPong(const QByteArray pingPL) const;
    void sendVersion();
    QByteArray CreateVersionPL();
    void sendVerAck();
    void sendGetData(const QByteArray inventory);
    void ProccessReceivedTX(const QByteArray theTxMsg);
    void sendAgetData();
    RawMessage* newMSG;

signals:
    void sendGlobalMSG(const QByteArray data) const;

public slots:
    void HandleGlobalMSG(QByteArray data);
    void handleGlobalTimer();

};
