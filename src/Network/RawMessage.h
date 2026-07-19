#pragma once

#include <QObject>

class RawMessage : public QObject
{
    Q_OBJECT
public:
    explicit RawMessage(const QByteArray theData, QObject *parent = nullptr);
    static QByteArray ExtractCommand(const QByteArray data);
    const static QByteArray MagicWord;
    void appendData(const QByteArray newData);
    bool checkCompleteness();
    QByteArray data;
    bool Filled=false;
    bool MessageProccessed = false;
private:

    QByteArray magicWord;
    QByteArray Command;
    QByteArray checksum;
    int PayloadSize;
    QByteArray payload;
    void AnalyzeMessage();


signals:
    void HandleMessage(const QByteArray data);
};

