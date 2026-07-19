#include "dnsLockUp.h"
#include <QHostAddress>
#include <QDebug>
#include <QTimer>

QList<QPair<QString, QString> > dnsLockUp::KnownHosts={};
dnsLockUp::dnsLockUp(QObject *parent)
    : QObject{parent}
{
    DNSLocker = QSharedPointer<QDnsLookup>(new QDnsLookup , &QObject::deleteLater);
    startInitialLockUp();
    QTimer::singleShot(2000,[](){});
    qDebug() << "KnownHosts :" << KnownHosts.length();
    for (const auto &x : qAsConst(KnownHosts)){
        qDebug() << x.second;
    }

QObject::connect(DNSLocker.get(), &QDnsLookup::finished, [&](){
        onDnsLookupFinished(DNSLocker.get());

    });
}

void dnsLockUp::onDnsLookupFinished(QDnsLookup *dnsLookup)
{
if (dnsLookup->error() != QDnsLookup::NoError) {
        qWarning() << dnsLookup->name() << " DNS lookup failed:" << dnsLookup->errorString();
    } else {
        const auto records = dnsLookup->hostAddressRecords();
        qDebug() << "Found" << records.count() << "IP addresses (seeds):";
        for (const QDnsHostAddressRecord &record : records) {
            //qDebug() << record.value().toString();// << " (TTL:" << record.timeToLive() << "s)";
            QPair<QString, QString> aPair ={"IP" , record.value().toString()};
            KnownHosts.append(aPair);
        }
        for (const auto &x : qAsConst(KnownHosts))
            qDebug() << "Host :" << x.second;
    }
}

bool dnsLockUp::start1Lockup(const QString &name)
{
    DNSLocker->setType(QDnsLookup::A);
    DNSLocker->setName(name);
    qDebug() << "Starting DNS lookup for seeds..." << name;
    DNSLocker->lookup();
    return true;
}

void dnsLockUp::startInitialLockUp()
{
    for(const QString &seed : qAsConst(this->Seeds)){
        start1Lockup(seed);
        QTimer::singleShot(2000,[](){});
    }
}
