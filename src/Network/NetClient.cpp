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
// Function to perform a single SHA-256 hash
std::vector<unsigned char> single_sha256(const unsigned char* data, size_t length) {
    std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, length);
    SHA256_Final(hash.data(), &sha256);
    return hash;
}

// Function to perform a double SHA-256 hash (HASH256)
std::vector<unsigned char> double_sha256(const unsigned char* data, size_t length) {
    // First hash
    std::vector<unsigned char> hash1 = single_sha256(data, length);

    // Second hash on the result of the first hash (which is always 32 bytes)
    std::vector<unsigned char> hash2 = single_sha256(hash1.data(), hash1.size());

    return hash2;
}

// Helper function to convert a byte vector to a hexadecimal string for display
std::string bytes_to_hex_string(const std::vector<unsigned char>& bytes) {
    std::stringstream ss;
    for (unsigned char byte : bytes) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

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
    //dnsLockUp* dns = new dnsLockUp;
    //this->initiateoutSocket();
    const auto versionMSG=buildVersionMsg();
}

void NetClient::sendMessage(QByteArray data)
{
    txSocket->write(data);
}

void NetClient::startHandShake()
{
    const auto versionMSG=buildVersionMsg();
    //sendMessage(versionMSG);
}

QByteArray NetClient::buildVersionMsg()
{
    qDebug() << "Building HASH";
    QByteArray data;
    data.append("F9BEB4D9"); // magic word
    data+= "76657273696F6E0000000000"; // "version"
    data += "55000000"; //Size
    data += "2C2F86F3"; // checksum
    //data = "F9BEB4D976657273696F6E0000000000550000002C2F86F3"; // fe687685ce5b
    QByteArray payLoad = "7E1101000000000000000000C515CF6100000000000000000000000000000000000000000000FFFF2E13894A208D000000000000000000000000000000000000FFFF7F000001208D00000000000000000000000000";
    //QByteArrayView view_payLoad(payLoad);
    const unsigned char* input_data = reinterpret_cast<const unsigned char*>(payLoad.constData());
    size_t input_length = payLoad.length();
     std::vector<unsigned char> final_hash = double_sha256(input_data, input_length);
    std::string hex_hash = bytes_to_hex_string(final_hash);
    const auto hash1 = sha256_2(QByteArray::fromHex(payLoad).toStdString());
    QByteArray hash_0 = QByteArray::fromStdString(hash1);
    const auto hash2 = sha256_2(QByteArray::fromHex(hash_0).toStdString()); // 2C2F86F3
    qDebug() << "HASH :" << hash1 <<  "//" << hash2 << " /////" ;

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

    txSocket->connectToHost("69.250.215.150",8333); // 69.250.215.150 , 89.125.48.42 , 86.201.225.172

}

