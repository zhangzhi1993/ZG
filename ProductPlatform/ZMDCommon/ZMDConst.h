#ifndef ZMDCONST_H
#define ZMDCONST_H

#include <QObject>

const int cPortClient_Server = 6001;
const int cPortClient_Proxy = 6003;
const int cPortServer_Proxy = 6004;
const int cPortUDPBroadCast = 3333;
const QList<int> cPortUDPBroadCasts = QList<int>() << 3330 << 3331 << 3332 << 3333 << 3334 << 3335 << 3336;

const QString c_Ready = "Ready";
const QString c_Separator = "|";

// ��Ϣ����
enum MSGTYPE
{
    mtTask,
    mtResult,
    mtQuery
};

// ��Ϣ��������
enum INFOTYPE
{
    infString
};

#endif // ZMDCONST_H
