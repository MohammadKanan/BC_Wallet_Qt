#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QDnsLookup>
#include <QPair>
#include <QList>

class dnsLockUp : public QObject
{
    Q_OBJECT
public:
    explicit dnsLockUp(QObject *parent = nullptr);
    QSharedPointer<QDnsLookup> DNSLocker;
    static QList<QPair<QString, QString> > KnownHosts;
    void onDnsLookupFinished(QDnsLookup* dnsLookup);
    bool start1Lockup(const QString& name);
    void startInitialLockUp();

private:
    QString Pieter = "seed.bitcoin.sipa.be";
    QString Luke = "dnsseed.bitcoin.dashjr.org";
    QString Provoost = "seed.bitcoin.sprovoost.nl";
    QStringList Seeds{Luke, Pieter , Provoost};


signals:
};
