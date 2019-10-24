#ifndef ZMDCONST_H
#define ZMDCONST_H

#include <QObject>

const int cPortClient_Server = 6001;
const int cPortClient_Proxy = 6003;
const int cPortServer_Proxy = 6004;
const int cPortUDPBroadCast = 3333;
const QList<int> cWorkerPortUDPBroadCasts = QList<int>() << 3330 << 3331 << 3332 << 3333 << 3334 << 3335 << 3336
                                                         << 3337 << 3338 << 3339;
const QList<int> cClientPortUDPBroadCasts = QList<int>() << 5550 << 5551 << 5552 << 5553 << 5554 << 5555 << 5556;

const QString c_Ready = "Ready";
const QString c_UnUsed = "UnUsed";
const QString c_Delete = "Delete";
const QString c_Separator = "|";

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

#endif // ZMDCONST_H
