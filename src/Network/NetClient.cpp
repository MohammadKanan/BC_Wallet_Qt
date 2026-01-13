#include "NetClient.h"
#include "src/Network/dnsLockUp.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
//
#include <iostream>
#include <string>

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
//
std::string hexSHA256(QByteArray input){
    std::string strHex = input.data();
    return strHex;
}

// Compute SHA-256 hash for a given string
std::string sha256(const std::string str)  {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

NetClient::NetClient(QObject *parent)
    : QObject{parent}
{
    dnsLockUp* dns = new dnsLockUp;
    this->initiateoutSocket();
}

void NetClient::sendMessage(QByteArray data)
{
    txSocket->write(data);
}

void NetClient::startHandShake()
{
    const auto versionMSG=buildVersionMsg();
    sendMessage(versionMSG);
}

QByteArray NetClient::buildVersionMsg()
{
    QByteArray data;
    data.append("F9BEB4D9"); // magic word
    data+= "76657273696F6E0000000000"; // "version"
    data += "55000000"; //Size
    data += "2C2F86F3"; // checksum
    //data = "F9BEB4D976657273696F6E0000000000550000002C2F86F3"; // fe687685ce5b
    QByteArray payLoad = "7E1101000000000000000000C515CF6100000000000000000000000000000000000000000000FFFF2E13894A208D000000000000000000000000000000000000FFFF7F000001208D00000000000000000000000000";
    const auto hash1 = sha256(payLoad.data());
    const auto hash2 = sha256(hash1);
    qDebug() << "HASH :" << hash1 << " /////" << hash2;

    //unsigned char* hashResult;
    //SHA256((unsigned char*)in.data(),in.length(),hashResult);
    //int s = strlen((char*)hashResult);
    //unsigned char* hashFinal;
    //SHA256(hashResult , s , hashResult);
    //qDebug() << "HASH2 :" << hashFinal;
    data += payLoad ;
    const auto data2 = QByteArray::fromHex(data);
    return data2;

}

void NetClient::initiateoutSocket()
{
    txSocket = QSharedPointer<QTcpSocket>(new QTcpSocket , &QObject::deleteLater);
    QObject::connect(txSocket.get() , &QTcpSocket::connected , this , [this](){
        qDebug() << "Conn established!";
        startHandShake();
    });
    QObject::connect(txSocket.get() , &QTcpSocket::disconnected , this, [](){
        qDebug() << "Conn disconnect!";
    });
    QObject::connect(txSocket.get(), &QTcpSocket::readyRead , this, [&](){
        // Read data from the socket
        QByteArray data = txSocket->readAll();
        QByteArray hexAsciiData = data.toHex();
        qDebug() << "Data received:" << hexAsciiData;
        if(!verackSent){
            data = "F9BEB4D976657261636B000000000000000000005DF6E0E2";
            const auto data2 = QByteArray::fromHex(data);
            qDebug() << " Starting verAck............................";
            sendMessage(data2);
            verackSent = !verackSent;
        }
        else{
            const auto magicWord= data.left(4);
            qDebug() << "magicWord:" << magicWord.toHex();
            auto command = data.mid(4,12);
            const QString commandSTR = QString::fromUtf8(command);
            qDebug() << "Command:" << commandSTR;
            if(commandSTR.startsWith("inv"))
                qDebug() << "caught Inv command .........................";

            const auto size =data.mid(16,2).toInt();
            qDebug() << "payload size:" << size;
            const auto checksum = data.mid(18,4);
            qDebug() << "Checksum :" << checksum;
            const auto payload = data.mid(22,data.size()-1);
            qDebug() << "Payload :" << "Size:" << data.length() << "/"<< payload.toHex();
        }
    });
    QObject::connect(txSocket.get(), &QTcpSocket::errorOccurred , this , [&](){
        qDebug() << "Socket error:" << txSocket->errorString();
    });

    txSocket->connectToHost("89.125.48.42",8333); // 69.250.215.150 , 89.125.48.42 , 86.201.225.172

}

