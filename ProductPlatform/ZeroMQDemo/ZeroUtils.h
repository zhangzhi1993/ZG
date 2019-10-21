#ifndef ZEROUTILS_H
#define ZEROUTILS_H
#include <QObject>
#include "zmq.hpp"

// 消息类型
enum MSGTYPE
{
    mtTask,
    mtResult,
    mtQuery
};

// 消息内容类型
enum INFOTYPE
{
    infString
};

class MTMessage
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

class ZeroUtils
{
public:
    ZeroUtils();
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
};

#endif // ZEROUTILS_H
