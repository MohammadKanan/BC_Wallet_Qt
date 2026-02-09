#include "Connection.h"
#include "NetClient.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <sstream>
#include <iomanip>
//
Connection::Connection(QString PeerIPAddress, QObject *parent)
    : QObject{parent}
{
    Peer_IP = PeerIPAddress;
    setObjectName(this->Peer_IP);
    this->CreateConnection();
    QObject::connect(&connTimer , &QTimer::timeout , this , &Connection::startConnection);
    connTimer.setInterval(2000);
    connTimer.setSingleShot(false);
    connTimer.start();
}

std::string Connection::sha256_2(const std::string input)
{
    // Array to hold the 32-byte (256-bit) binary hash result
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // Perform the SHA256 computation using the OpenSSL function
    // SHA256(data, length, output_buffer)
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);

    // Convert the binary hash result to a hexadecimal string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}

void Connection::startConnection()
{
    qDebug() << "Timer checking connection For :" << Peer_IP;
    if(BC_Socket->state() == QAbstractSocket::ConnectedState){
        connTimer.stop();
    } else this->CreateConnection();
}

void Connection::CreateConnection()
{
    BC_Socket =  QSharedPointer<QTcpSocket>(new QTcpSocket , &QObject::deleteLater);
    QObject::connect(BC_Socket.get() , &QTcpSocket::connected , this , [this](){
        qDebug() << "Conn established!";
        this->sendVersion();
    });
    QObject::connect(BC_Socket.get() , &QTcpSocket::disconnected , this, [&](){
        qDebug() << "Conn disconnect!" << this->objectName();
    });
    QObject::connect(BC_Socket.get(), &QTcpSocket::readyRead , this , &Connection::ProccessSocket);

    BC_Socket->connectToHost(Peer_IP,8333); // 69.250.215.150 , 89.125.48.42 , 86.201.225.172
}

void Connection::ProccessSocket()
{
    QByteArray data = BC_Socket->readAll();
    //QByteArray hexAsciiData = data.toHex();
    const auto _magicWord= data.left(4);
     qDebug() << "magicWord:" << _magicWord.toHex();
    if(_magicWord.toHex() != this->MagicWord){
        qDebug() << "Wrong magic word .... dropping connection";
        connTimer.stop();
        BC_Socket->disconnectFromHost();
        return;
    }
    QByteArray command2 = data.mid(4,12);

    QByteArray command = NetClient::ExtractCommand(command2);
    const QString commandSTR = QString::fromUtf8(command);
    qDebug() << "Command:" << commandSTR ;
    if(commandSTR.startsWith("inv")){
        isINV = true;
    }
    const auto size =data.mid(16,4).toHex();
    qDebug() << "payload size:" << size;
    MSG_checksum = data.mid(20,4).toHex();
    //qDebug() << "Checksum :" << checksum;
    const auto payload = data.mid(24,data.size()-1);
    qDebug() << "Payload :" << "Size:" << payload.length() ;
    if (commandSTR.startsWith("ping")) this->sendPong(payload);
    else if(commandSTR.startsWith("inv")) sendGetData(payload);
    else if ((commandSTR.startsWith("verack") &&!verackSent) && versionReceived) {
        verAckreceived = true;
        qDebug() << "verAck received .....";
        sendVerAck();
    } else if(commandSTR.startsWith("version" ) &&!verackSent && !versionReceived){
        versionReceived = true;
        qDebug() << "version received .........";
        sendVerAck();
    }


}

void Connection::sendMessage(QByteArray data) const
{
    BC_Socket->write(data);
}

void Connection::sendPong(const QByteArray pingPL) const
{
    QByteArray Command = ("pong00000000");
    QByteArray _Command = Command;
    QByteArray GetDataHeader;
    GetDataHeader.append(QByteArray::fromHex(MagicWord));
    GetDataHeader.append(_Command);
    bool ok;
    const auto _Count = pingPL.length();
    const auto count = QByteArray::number(_Count , 16);
    auto sizeHex = count;
    qDebug() << "pong Size array :" <<sizeHex << "/" <<  count << "/" << _Count;
    while (sizeHex.length() < 8){
        sizeHex.prepend("0"); // make it 4 bytes
    }
    NetClient::ToLittleEndian(&sizeHex);
    qDebug() << "pong payload size:" << sizeHex << count;
    GetDataHeader.append(QByteArray::fromHex(sizeHex));
    //GetDataHeader.append(QByteArray::fromStdString(sha256_2(QByteArray::fromHex(invData).toStdString())).left(8));
    //GetDataHeader.append(QByteArray::fromStdString(sha256_2((thePing).toStdString())).left(8));
    //
    GetDataHeader.append(pingPL);
    sendMessage(GetDataHeader);
}

void Connection::sendVersion()
{
    QByteArray data;
    data.append(MagicWord); // magic word
    data+= "76657273696F6E0000000000"; // "version command"
    const auto PL = CreateVersionPL();
    // size
    const auto size = PL.length();
    auto sizeHex = QByteArray::number(size/2 , 16);
    //qDebug() << "Size array :" <<sizeHex << "/" <<  sizeHex.length();
    while (sizeHex.length() < 8){
        sizeHex.append("0"); // make it 4 bytes
    }
    //qDebug() << "Size array final :" <<sizeHex << "/" <<  sizeHex.length();
    data += sizeHex;
    // checksum
    //QByteArray payLoad = "7E1101000000000000000000C515CF6100000000000000000000000000000000000000000000FFFF2E13894A208D000000000000000000000000000000000000FFFF7F000001208D00000000000000000000000000";
    const auto hash1 = QByteArray::fromStdString(sha256_2(QByteArray::fromHex(PL).toStdString()));
    const auto hash2 = sha256_2(QByteArray::fromHex(hash1).toStdString()); // 2C2F86F3
    const QByteArray _checksum = QByteArray::fromStdString(hash2).left(8);
    data += _checksum; // checksum
    //qDebug() << "HASH :" << hash1 <<  "//" << hash2 << " /////" << _checksum ;
    data += PL ;
    const auto data2 = QByteArray::fromHex(data);
    sendMessage(data2);
}

QByteArray Connection::CreateVersionPL()
{
    QByteArray payLoad;
    payLoad.append("7E1101");
    auto _services = "0000000000000000";
    payLoad.append(_services);
    // LE time
    auto _now = QDateTime::currentDateTime().toSecsSinceEpoch();
    QByteArray _nowHex = QByteArray::number(_now,16);
    //qDebug() << "LE Before :" << _nowHex;
    NetClient::ToLittleEndian(&_nowHex);
    //qDebug() << "LE Time :" << _nowHex <<  "/";
    payLoad.append(_nowHex);
    // Remote Services
    const auto remote_Services = "0000000000000000";
    payLoad.append(remote_Services);
    // Remote IP
    const auto rem_IP = "00000000000000000000FFFF2E13894A";
    payLoad.append(rem_IP);
    const auto rem_Port = "208D";
    payLoad.append(rem_Port);
    const auto local_Services = "0000000000000000";
    payLoad.append(local_Services);
    const auto local_IP = "00000000000000000000FFFF7F000001";
    payLoad.append(local_IP);
    const auto local_Port = "208D";
    payLoad.append(local_Port);
    payLoad.append("00000000000000000000000000");
    return payLoad;
    const auto Nonce = "0000000000000000";
    payLoad.append(Nonce);
    const auto User_Agent = "00";
    payLoad.append(User_Agent);
    const auto Last_Block = "0000";
    payLoad.append(Last_Block);
    //qDebug() << "PAYLOAD :" << payLoad;
}

void Connection::sendVerAck()
{
    QByteArray data1 = "F9BEB4D976657261636B000000000000000000005DF6E0E2";
    const auto data2 = QByteArray::fromHex(data1);
    qDebug() << " Sending verAck............................";
    sendMessage(data2);
    verackSent = true;
}

void Connection::sendGetData(const QByteArray inventory)
{
    // Header
    QByteArray Command = ("getdata00000");
    QByteArray GetDataHeader;
    GetDataHeader.append(QByteArray::fromHex(MagicWord));
    GetDataHeader.append(Command);
    bool ok;
    const auto size = inventory.length();
    qDebug() << "GD size:" << size;
    auto sizeHex = QByteArray::number(size , 16);
    qDebug() << "Size array :" <<sizeHex << "/" <<  sizeHex.length();
    while (sizeHex.length() < 8){
        sizeHex.prepend("0"); // make it 4 bytes
    }
    NetClient::ToLittleEndian(&sizeHex);
    qDebug() << "getData payload size:" << sizeHex;
    GetDataHeader.append(QByteArray::fromHex(sizeHex));
    qDebug() << "getdata checksum is :" << MSG_checksum;
    GetDataHeader.append((QByteArray::fromHex(MSG_checksum)));
    //
    GetDataHeader.append(inventory);
    qDebug() << "GetData :" << GetDataHeader.toHex();
    sendMessage(GetDataHeader);
}
