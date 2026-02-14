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
    globalTimer.setInterval(2000);
    globalTimer.setSingleShot(false);
    globalTimer.start();
}

std::string Connection::sha256_2(const std::string input) const
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

void Connection::ToLittleEndian(QByteArray *ba)
{
    std::reverse(ba->begin(), ba->end());
    for (int i = 0 ; i < ba->length() ; i+=2){
        std::reverse(ba->begin()+i, ba->begin()+i+2);
    }
}

void Connection::startConnection()
{
    qDebug() << "Timer checking connection For :" << Peer_IP;
    if(BC_Socket->state() == QAbstractSocket::ConnectedState || this->abortPeer){
        connTimer.stop();
    } else this->CreateConnection();
}

void Connection::CreateConnection()
{
    BC_Socket =  QSharedPointer<QTcpSocket>(new QTcpSocket , &QObject::deleteLater);
    QObject::connect(BC_Socket.get() , &QTcpSocket::connected , this , [this](){
        qDebug() << "Conn established! ... " << this->objectName();
        this->sendVersion();
    });
    QObject::connect(BC_Socket.get() , &QTcpSocket::disconnected , this, [&](){
        qDebug() << "Conn disconnect!" << this->objectName();
    });
    QObject::connect(BC_Socket.get(), &QTcpSocket::readyRead , this , &Connection::ReadSocket);

    BC_Socket->connectToHost(Peer_IP,8333); // 69.250.215.150 , 89.125.48.42 , 86.201.225.172
}

void Connection::ReadSocket()
{
    QByteArray data = BC_Socket->readAll();
    if(data.length() < GetPayloadSizeFromHeader(data)) //
        qDebug() << "Incomplete commnd .... wait...";
    SplitMultipleCommands(data);
    //qDebug() << " Received message :" << hexAsciiData;


}

int Connection::GetPayloadSizeFromHeader(const QByteArray data)
{
     auto size =data.mid(16,4).toHex();
    Connection::ToLittleEndian(&size);
     bool ok;
    const auto payloadSize = size.toInt(&ok,16);
    qDebug() << "payload size:" << payloadSize;
}

void Connection::SplitMultipleCommands(const QByteArray data)
{
    QByteArray hexAsciiData = data.toHex();
    //qDebug() << " Received message :" << hexAsciiData;
    const auto _magicWord = data.left(4);
    qDebug() << "magicWord:" << _magicWord.toHex() << " ......" << this->objectName();
    int counter = data.count(_magicWord);
    if( counter > 1){
        qDebug() << "Found multiple commands ...." ;
        int pos = data.indexOf(_magicWord);
        while (pos != -1) {
            int lastPos = pos;
            pos = data.indexOf(_magicWord, pos + _magicWord.length()); // Search from next position
            ProccessIncomingCommand(data.mid(lastPos,pos-lastPos));

            //ProccessIncomingCommand(data.mid(lastPos,data.length()-1));
        }

    }  else ProccessIncomingCommand(data);


}

void Connection::ProccessIncomingCommand(const QByteArray data)
{
    const auto _magicWord = data.left(4);
    qDebug() << "magicWord:" << _magicWord.toHex() << " ......" << this->objectName();
    if(_magicWord != QByteArray::fromHex(this->MagicWord)){
        qDebug() << "Wrong magic word .... dropping connection" << " ......" << this->objectName();
        this->abortPeer=true;
        connTimer.stop();
        //BC_Socket->disconnectFromHost();
        //return;
    }
    QByteArray command2 = data.mid(4,12);

    QByteArray command = NetClient::ExtractCommand(command2);
    const QString commandSTR = QString::fromUtf8(command);
    qDebug() << "Command:" << commandSTR << " ......" << this->objectName();
    if(commandSTR.startsWith("inv")){
        isINV = true;
    }
    const auto size =data.mid(16,4).toHex();
    //qDebug() << "payload size:" << size;
    MSG_checksum = data.mid(20,4).toHex();
    //qDebug() << "Checksum :" << checksum;
    const auto payload = data.mid(24,data.size()-1);
    qDebug() << "Payload :" << "Size:" << payload.length() ;
    if (commandSTR.startsWith("ping")) this->sendPong(payload);
    else if(commandSTR.startsWith("inv")) sendGetData(payload);
    else if ((commandSTR.startsWith("verack") &&!verackSent) && versionReceived) {
        verAckreceived = true;
        qDebug() << "verAck received ....." << " ......" << this->objectName();
        sendVerAck();
    } else if(commandSTR.startsWith("version" ) &&!verackSent && !versionReceived){
        versionReceived = true;
        qDebug() << "version received ........."<< payload.toHex() << this->objectName();
        sendVerAck();
    } else if(commandSTR.startsWith("tx" )) ProccessReceivedTX(payload);
    //else if (verackSent &&  commandSTR.startsWith("feefilter")) sendAgetData();

}

void Connection::sendMessage(const QByteArray data) const
{
    BC_Socket->write(data);
}

void Connection::sendPong(const QByteArray pingPL) const
{
    qDebug() << "Pong checksum :" << this->MSG_checksum;
    QByteArray Command = ("pong00000000");
    QByteArray hexCommand = "706f6e670000000000000000";
    QByteArray PongMsg;
    PongMsg.append(MagicWord);
    PongMsg.append(hexCommand);
    bool ok;
    const auto _Count = pingPL.length();
    const auto count = QByteArray::number(_Count , 16);
    auto sizeHex = count;
    qDebug() << "pong Size array :" <<sizeHex << "/" <<  count << "/" << _Count;
    while (sizeHex.length() < 8){
        sizeHex.prepend("0"); // make it 4 bytes
    }
    Connection::ToLittleEndian(&sizeHex);
    //qDebug() << "pong payload size:" << sizeHex << count;
    PongMsg.append((sizeHex));
    const auto hash1 = QByteArray::fromStdString(sha256_2(pingPL.toStdString()));
    const auto hash2 = sha256_2(QByteArray::fromHex(hash1).toStdString()); // 2C2F86F3
    const QByteArray _checksum = QByteArray::fromStdString(hash2).left(8);
    qDebug() << "Pong generated checksum :" << _checksum;
    PongMsg.append(_checksum);
    //
    QByteArray pg = QByteArray::fromHex(PongMsg);
    pg.append(pingPL);
    qDebug() << "png msg ..." << pg.toHex();
    sendMessage(pg);
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
    const auto VersionMSG = QByteArray::fromHex(data);
    qDebug() << "sent version msg :" << VersionMSG.toHex();
    sendMessage(VersionMSG);
}

QByteArray Connection::CreateVersionPL()
{
    QByteArray payLoad;
    payLoad.append("801101"); //7E1101
    auto _services = "0000000000000000";
    payLoad.append(_services);
    // LE time
    auto _now = QDateTime::currentDateTime().toSecsSinceEpoch();
    QByteArray _nowHex = QByteArray::number(_now,16);
    //qDebug() << "LE Before :" << _nowHex;
    Connection::ToLittleEndian(&_nowHex);
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
    const auto VerackMSG = QByteArray::fromHex(data1);
    qDebug() << " Sending verAck............................" << " ......" << this->objectName();
    sendMessage(VerackMSG);
    verackSent = true;
    handShakeComplete = true;
}

void Connection::sendGetData(const QByteArray inventory)
{
    // Header
    QByteArray Command = "getdata00000";
    QByteArray hexCommand = "676574646174610000000000";
    QByteArray GetDataMSG;
    GetDataMSG.append((MagicWord));
    GetDataMSG.append(hexCommand);
    bool ok;
    const auto size = inventory.length();
    qDebug() << "GD size:" << size;
    auto sizeHex = QByteArray::number(size , 16);
    qDebug() << "Size array :" <<sizeHex << "/" <<  sizeHex.length() << " ......" << this->objectName();
    while (sizeHex.length() < 8){
        sizeHex.prepend("0"); // make it 4 bytes
    }
    Connection::ToLittleEndian(&sizeHex);
    //qDebug() << "getData payload size:" << sizeHex;
    GetDataMSG.append(sizeHex);
    //qDebug() << "getdata checksum is :" << MSG_checksum;

    const auto hash1 = QByteArray::fromStdString(sha256_2(inventory.toStdString()));
    const auto hash2 = sha256_2(QByteArray::fromHex(hash1).toStdString());
    const QByteArray _checksum = QByteArray::fromStdString(hash2).left(8);
    //qDebug() << "Local checksum :" << _checksum;
    GetDataMSG.append(_checksum);
    //
    QByteArray GD = QByteArray::fromHex(GetDataMSG);
    GD.append(inventory);
    //qDebug() << "GetData :" << GD.toHex() << " ......" << this->objectName();
    sendMessage( (GD));
    //emit sendGlobalMSG(GD);
}

void Connection::ProccessReceivedTX(const QByteArray theTxMsg)
{
    qDebug() << "TX :" << theTxMsg.toHex();
    const auto txVer = theTxMsg.mid(0,4);
    qDebug() << "Tx version :" << txVer.toHex();
    const auto count  = theTxMsg.mid(4,1);
    bool ok;
    const auto inputCount = count.toHex().toInt(&ok,16);
    qDebug() << "Input count :" << inputCount;
    int _LoopCounter=5;
    // inputs
    for (int looper=1 ; looper <= inputCount ; looper++){
        //_LoopCounter += 5;
    const auto tx1 = theTxMsg.mid( _LoopCounter ,32);
    qDebug() << " TX ID :" << tx1.toHex();
    _LoopCounter += 32;
    const auto VOUT = theTxMsg.mid(_LoopCounter ,4);
    //Connection::ToLittleEndian(&VOUT);
    qDebug() << "Value out :" << VOUT.toHex();
    _LoopCounter += 4;
    const auto scriptsigsize = theTxMsg.mid(_LoopCounter , 1);
    qDebug() << "scriptsigsize hex :" << scriptsigsize.toHex();
    const auto scriptsigsize_Count = scriptsigsize.toHex().toInt(&ok,16);
    qDebug() << "scriptsigsize :" << scriptsigsize_Count;
    _LoopCounter += 1;
    const auto scriptsig = theTxMsg.mid(_LoopCounter , scriptsigsize_Count);
    qDebug() << "scriptsig :" << scriptsig.toHex();
    _LoopCounter += scriptsigsize_Count;
    const auto Sequence = theTxMsg.mid(_LoopCounter , 4 );
    _LoopCounter += 4;
    qDebug() << "Sequence : " << Sequence.toHex();
    }
    // outouts
    const auto O_count  = theTxMsg.mid(_LoopCounter,1);
    _LoopCounter += 1;
    const auto outputCount = O_count.toHex().toInt(&ok,16);
    qDebug() << "Output count : " << outputCount;
    for (int looper =1 ; looper <= outputCount ; looper ++){
       const auto amount = theTxMsg.mid(_LoopCounter ,8);
       _LoopCounter += 8;
       qDebug() << "  amount Value :" << amount.toHex();
       const auto ScriptPubKeySize =  theTxMsg.mid(_LoopCounter ,1).toHex().toInt(&ok,16);
       qDebug() << "scriptpubkeysize :" << ScriptPubKeySize;
       _LoopCounter += 1;
       const auto ScriptPubKey = theTxMsg.mid(_LoopCounter ,ScriptPubKeySize);
       _LoopCounter += ScriptPubKeySize;
       qDebug() << "ScriptPubKey :" <<ScriptPubKey.toHex();
    }
    const auto LockTime = theTxMsg.mid(_LoopCounter ,theTxMsg.length()-1).toHex();
    if(theTxMsg.mid(_LoopCounter,theTxMsg.length()-1).length() > 4 ) qDebug() << " very long locktime ..............";
    qDebug() << "LockTime :" <<  LockTime;

}

void Connection::sendAgetData()
{
    qDebug() << "Sending auxilliary getdata ........";
    const QByteArray data = "f9beb4d9676574646174610000000000210100003854a56e0801000000100e755d6ff0c3d30d5575ae656ae2c1ceeb036fdc72eb976e03f075acd068ff01000000feae03471b397bb6589bc4c3f9472cc2a4a7344648a93b1b79342dd957a74a5101000000ed046c655721ff20cdcc638a32af6b9bdeb55ae2081713046d695d3f150514e2010000004e9686c8fec1446510a469da9ee9b0407f55982bffaf4fdbed749af70f2a59e801000000026414a5fc339eb3a029baba43890b2d3383f66c6757783f5df2c67f29508ddf01000000a802ddd824db8a1c2b426ea1a18c312881fbe2ec9aad082e6d15252a4c11e54d010000004cb9a76511a874057cf7cb2c14a9fc75fedafca3487e2b78845537eea087ffaa0100000006a6aeda9e28704354ee266c94a103e41f1c68f4d15fcef874d5c23ef64481d9";
    sendMessage(QByteArray::fromHex(data));
}

void Connection::HandleGlobalMSG(QByteArray data)
{
    qDebug() << " trying global msg .....";
    this->globalMSG = data;
    if(BC_Socket->state() == QAbstractSocket::ConnectedState && !this->abortPeer &&handShakeComplete){
        this->sendMessage(data);
        qDebug() << "Sending global msg ..." << this->objectName();
        globalTimer.stop();
    }
    else {
        qDebug() << "Node not connected ...aborting global msg ..." << this->objectName();
        if(!globalTimer.isActive())
            globalTimer.start();
    }
}

void Connection::handleGlobalTimer()
{
    qDebug() << " global timer ... ...";
    this->HandleGlobalMSG(globalMSG);
}
