#ifndef ZMDUTILS_H
#define ZMDUTILS_H
#include <QObject>
#include "zmq.hpp"
#include "zmdcommon_global.h"
class MTMessage;
class ZMDCOMMONSHARED_EXPORT ZMDUtils
{

public:
    ZMDUtils();

    static bool readFileString(QString path, QByteArray &data);
    static bool writeFileString(QString path, const QByteArray &data);
    static bool send(zmq::socket_t *socket, QString msg);
    static bool sendmore(zmq::socket_t *socket, QString msg);
    static QString resv(zmq::socket_t *socket);
    static QString lastAddr(QString addr);
    static bool sendMsg(zmq::socket_t *socket, MTMessage mtMsg);
    static MTMessage resvMsg(zmq::socket_t *socket);
    static QString refreshAddr(QString addr);
    static void printHighWaterLevel(zmq::socket_t *socket);
    static void setHighWaterLevel(zmq::socket_t *socket, int highWaterLevel);
    static QByteArray resvByteArr(zmq::socket_t *socket);
    static bool sendByteArr(zmq::socket_t *socket, QByteArray msg);
    static bool tryConnect(zmq::socket_t *socket, QString addr);
};

#endif // ZMDUTILS_H
