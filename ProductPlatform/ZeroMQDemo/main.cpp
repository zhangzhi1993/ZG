#include "zmq.hpp"
#include "czmq.h"

#include <QCoreApplication>
#include "zmq.hpp"
#include "qdebug.h"
#include <iostream>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/event_struct.h>
#include <QString>
#include <QtConcurrent>
#include "ZeroUtils.h"
#include "zhelpers.h"

/// DEALER-ROUTER-ROUTER-DEALER
/// 经由代理处理Client和Server之间的交互，且手动实现负载平衡

//#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib, "wsock32.lib")
//#pragma comment(lib, "Advapi32.lib")

//QString g_IP = "localhost";
//int g_WorkerCnt = 3;
//int g_TaskCnt = 10;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    std::string str1, str2, str3;
    qDebug() << QStringLiteral("请输入启动ip地址");
    std::cin >> str1;
    qDebug() << QStringLiteral("请输入Worker数目");
    std::cin >> str2;
    qDebug() << QStringLiteral("请输入Task数目");
    std::cin >> str3;


    QString g_IP = QString::fromStdString(str1);
    int g_WorkerCnt = QString::fromStdString(str2).toInt();
    int g_TaskCnt = QString::fromStdString(str3).toInt();

    QtConcurrent::run(CreateUtils::createRouterProxy);

    for (int i = 0; i < g_WorkerCnt; i++)
    {
        QString uUID = QUuid::createUuid().toString();
        QtConcurrent::run(CreateUtils::createWorker, uUID, g_IP);
    }

    QString uUID = QUuid::createUuid().toString();
    QtConcurrent::run(CreateUtils::createClient, uUID, g_IP, g_TaskCnt);

    return a.exec();
}


