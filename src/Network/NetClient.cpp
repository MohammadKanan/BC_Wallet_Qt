#include "NetClient.h"
#include "src/Network/dnsLockUp.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
//
#include <iostream>
#include <string>
#include <QtEndian>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
//
// Function to perform a single SHA-256 hash
QString NetClient::Peer_IP = "84.247.143.204";
std::string sha256_2(const std::string input) {
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
    //buildPayload();
}

void NetClient::sendMessage(QByteArray data) const
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
    //qDebug() << "Building HASH";
    QByteArray data;
    data.append(MagicWord); // magic word
    data+= "76657273696F6E0000000000"; // "version command"
    const auto PL = buildVersinMSG_Payload();
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
    return data2;

}

QByteArray NetClient::buildVersinMSG_Payload()
{
    // version ..
    QByteArray payLoad;
    payLoad.append("7F1101");
    auto _services = "0000000000000000";
    payLoad.append(_services);
    // LE time
    auto _now = QDateTime::currentDateTime().toSecsSinceEpoch();
    QByteArray _nowHex = QByteArray::number(_now,16);
    char frameBuffer[256];
    qToLittleEndian(_nowHex, frameBuffer);
    const auto result = QByteArray::fromRawData(reinterpret_cast<const char*>(frameBuffer), 16);
    //qDebug() << "LE Before :" << _nowHex;
    ToLittleEndian(&_nowHex);
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
    return payLoad;
}

void NetClient::ToLittleEndian(QByteArray *ba)
{
    std::reverse(ba->begin(), ba->end());
    for (int i = 0 ; i < ba->length() ; i+=2){
         std::reverse(ba->begin()+i, ba->begin()+i+2);
    }
}
QByteArray NetClient::ExtractCommand(const QByteArray data) const
{
    QByteArray command = QByteArray::fromRawData(data, data.length());
    QByteArray::iterator iter = command.begin();
    char fixed[255];
    int index = 0;
    while(iter != command.end())
    {
        QChar c = *iter;
        if (c != '\0') fixed[index++] = c.toLatin1();
        iter++;
    }
    fixed[index] = '\0';
    return fixed;
}

void NetClient::ProccessInvMsg(const QByteArray invArray) const // Bytes
{
    qDebug() << "inv payload :" << invArray.toHex();
    bool ok;
    const auto _Count = invArray.toHex().left(2);
    const auto count = _Count.toInt(&ok,16);
    qDebug() << "Count = " << count;
    const auto invHash = invArray.toHex().mid(2,invArray.toHex().length()-1);
    createGetData(invArray);
    /*
    for(int i = 0 ; i < count ; i++){
        int j = i*72;
        QByteArray HASH = invHash.mid(j,72);

        int index = HASH.indexOf("01000000");
        HASH.replace(index,8,"01000040");
        HASH.prepend("01");
        qDebug() << " HASH " << i << "/" << HASH;
        HASH.prepend("01");
        createGetData(HASH);
    }
    */

}

void NetClient::createGetData(const QByteArray countPlushash) const //Bytes
{

    const auto _header = constructGetDataHeader(countPlushash);
    qDebug() << "getdata header :" << _header;
    auto MSG = QByteArray::fromHex(_header );
    MSG.append(countPlushash);
    qDebug() << "GetData :" << MSG;
    sendMessage(MSG);
}

QByteArray NetClient::constructGetDataHeader(const QByteArray invData) const // bytes
{
    QByteArray Command ="getdata";
    //
    while(Command.length() < 12)
        Command.append("0");
    Command = ("getdata00000");
    QByteArray _Command = QByteArray::fromHex(Command);
    QByteArray GetDataHeader;
    GetDataHeader.append(MagicWord);
    GetDataHeader.append(_Command);
    bool ok;
    const auto _Count = invData.toHex().left(2);
    const auto count = _Count.toInt(&ok,16);
    //const auto Size = invData.length();
    auto sizeHex = QByteArray::number(count);
    //qDebug() << "Size array :" <<sizeHex << "/" <<  sizeHex.length();
    while (sizeHex.length() < 4){
        sizeHex.append("0"); // make it 4 bytes
    }
    qDebug() << "getData payload size:" << sizeHex;
    GetDataHeader.append(sizeHex);
    //GetDataHeader.append(QByteArray::fromStdString(sha256_2(QByteArray::fromHex(invData).toStdString())).left(8));
    //GetDataHeader.append(QByteArray::fromStdString(sha256_2((invData).toStdString())).left(8));
    return GetDataHeader;

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
        bool isINV = false;
        QByteArray data = txSocket->readAll();
        QByteArray hexAsciiData = data.toHex();
        //qDebug() << "Data received:" << hexAsciiData;
        if(!verackSent){
            QByteArray data1 = "F9BEB4D976657261636B000000000000000000005DF6E0E2";
            const auto data2 = QByteArray::fromHex(data1);
            qDebug() << " Starting verAck............................";
            sendMessage(data2);
            verackSent = !verackSent;
        }
        {
            const auto _magicWord= data.left(4);
            qDebug() << "magicWord:" << _magicWord.toHex();
            QByteArray command2 = data.mid(4,12);

            QByteArray command = ExtractCommand(command2);
            const QString commandSTR = QString::fromUtf8(command);
            qDebug() << "Command:" << commandSTR  << " / " << command;
            if(commandSTR.startsWith("inv")){
                isINV = true;
                qDebug() << "caught Inv command .........................";
            }

            const auto size =data.mid(16,4).toInt();
            qDebug() << "payload size:" << size;
            const auto checksum = data.mid(20,4).toHex();
            qDebug() << "Checksum :" << checksum;
            const auto payload = data.mid(24,data.size()-1);
            //qDebug() << "Payload :" << "Size:" << data.length() << "/"<< payload.toHex();
            if(isINV){
                isINV = false;
                ProccessInvMsg(payload);
            }
        }
    });
    QObject::connect(txSocket.get(), &QTcpSocket::errorOccurred , this , [&](){
        qDebug() << "Socket error:" << txSocket->errorString();
    });

    txSocket->connectToHost(NetClient::Peer_IP,8333); // 69.250.215.150 , 89.125.48.42 , 86.201.225.172

}



