#ifndef MTMESSAGE_H
#define MTMESSAGE_H
#include <QObject>
#include "zmq.hpp"
#include "zmdcommon_global.h"
enum MSGTYPE;
enum INFOTYPE;

class ZMDCOMMONSHARED_EXPORT MTMessage
{
public:
    MSGTYPE mtType;
    QString srcAddr;
    QString dstAddr;
    int total;
    int sequence;
    INFOTYPE infType;
    QByteArray info;

    QString toString();
};


#endif // MTMESSAGE_H
