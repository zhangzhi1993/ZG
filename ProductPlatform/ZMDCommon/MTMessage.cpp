#include "MTMessage.h"
#include <QFile>
#include <QDataStream>
#include <QUuid>
#include <QDebug>
#include <ZMDConst.h>
#include "ZMDUtils.h"

QString MTMessage::toString()
{
    QString sType;
    switch (mtType) {
    case mtTask:
        sType = "mtTask";
        break;
    case mtResult:
        sType = "mtResult";
        break;
    case mtQuery:
        sType = "mtQuery";
        break;
    default:
        break;
    }
    return QString("%1 %2 %3").arg(sType).arg(ZMDUtils::lastAddr(srcAddr))
            .arg(ZMDUtils::lastAddr(dstAddr)) + (infType == infString ? QString(info) : "");
}

bool MTMessage::operator ==(const MTMessage &right)
{
    return right.uUID == this->uUID;
}
