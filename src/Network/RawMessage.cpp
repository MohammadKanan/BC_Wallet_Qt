#include "RawMessage.h"
#include "NetClient.h"
#include <QObject>
#include <QDebug>
#include <QTimer>

const QByteArray RawMessage::MagicWord = "F9BEB4D9";

RawMessage::RawMessage(const QByteArray theData, QObject *parent)
    : QObject{parent}
{
    this->data = theData;
    this->AnalyzeMessage();
}

QByteArray RawMessage::ExtractCommand(const QByteArray data)
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

void RawMessage::appendData(const QByteArray newData)
{
    this->payload.append(newData);
}

void RawMessage::AnalyzeMessage()
{
    const auto _magicWord = data.left(4);
    qDebug() << "magicWord:" << _magicWord.toHex() << " ......" << this->objectName();
    if(_magicWord != QByteArray::fromHex(RawMessage::MagicWord)){
        qDebug() << "Wrong magic word .... dropping connection" << " ......" << this->objectName();
        //BC_Socket->disconnectFromHost();
        //return;
    }
    QByteArray _command = data.mid(4,12);

    this->Command = RawMessage::ExtractCommand(_command);
    const QString commandSTR = QString::fromUtf8(this->Command);
    //qDebug() << "Command:" << commandSTR << " ......" << this->objectName();
    bool ok;
    auto size = data.mid(16,4).toHex();
    NetClient::ToLittleEndian(&size);
    this->PayloadSize = size.toInt(&ok,16);
    qDebug() << "Message payload size:" << this->PayloadSize;
    this->checksum = data.mid(20,4).toHex();
    //qDebug() << "Checksum :" << checksum;
    this->payload = data.mid(24,data.size()-1);
    //const auto size = this->payload.length();
    QTimer::singleShot(1000,this,&RawMessage::checkCompleteness);
    //checkCompleteness();
}

bool RawMessage::checkCompleteness()
{
    if(!this->MessageProccessed && (static_cast<int>(this->payload.length()) == this->PayloadSize)){
        qDebug() << "Message boiled for proccessing ....";
        this->Filled = true;
        MessageProccessed = true;
        emit HandleMessage(this->data);
        return true;
    }
    return false;
}
