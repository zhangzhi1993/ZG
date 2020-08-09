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
/// ���ɴ�����Client��Server֮��Ľ��������ֶ�ʵ�ָ���ƽ��

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
    qDebug() << QStringLiteral("����������ip��ַ");
    std::cin >> str1;
    qDebug() << QStringLiteral("������Worker��Ŀ");
    std::cin >> str2;
    qDebug() << QStringLiteral("������Task��Ŀ");
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


