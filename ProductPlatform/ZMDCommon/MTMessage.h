#ifndef MTMESSAGE_H
#define MTMESSAGE_H
#include <QObject>
#include "zmq.hpp"
#include "zmdcommon_global.h"
#include "ZMDConst.h"

class ZMDCOMMONSHARED_EXPORT MTMessage
{
public:
    MSGTYPE mtType;
    QString uUID;
    QString srcAddr;
    int synNo; //发出的序号
    QString dstAddr;
    int ackNo; //验证的序号
    int total;
    int sequence;
    INFOTYPE infType;
    QByteArray info;

    QString toString();

    bool operator ==(const MTMessage & right);
};


#endif // MTMESSAGE_H
