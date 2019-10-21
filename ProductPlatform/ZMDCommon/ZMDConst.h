#ifndef ZMDCONST_H
#define ZMDCONST_H

#include <QObject>
const int cPortClient_Server = 6001;
const int cPortClient_Proxy = 6003;
const int cPortServer_Proxy = 6004;

const QString c_Ready = "Ready";

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
