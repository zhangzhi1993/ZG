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
#include "ZMDUtils.h"
#include "MTMessage.h"
#include "zhelpers.h"
#include "ZMDConst.h"

/// DEALER-ROUTER-ROUTER-DEALER
/// 经由代理处理Client和Server之间的交互，且手动实现负载平衡

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "Advapi32.lib")

QString g_IP = "localhost";
int g_WorkerCnt = 3;

const int c_PipeLine = 5;

void createWorker(QString uUID)
{
    std::string sUID = uUID.toStdString();
    qDebug() << "createWorker" << uUID;

    zmq::context_t context(1);
    while (1)
    {
        // 任务处理器
        zmq::socket_t sckConsumer (context, ZMQ_DEALER);
        sckConsumer.setsockopt(ZMQ_IDENTITY, sUID.data(), sUID.size());
        sckConsumer.connect(QString("tcp://%1:%2").arg(g_IP).arg(cPortServer_Proxy).toStdString());

//        qDebug() << "createWorker" << uUID << ++nCnt;

        // 告诉代理已经准备好了
        ZMDUtils::send(&sckConsumer, c_Ready);

        // 获取请求
        MTMessage mtMsg = ZMDUtils::resvMsg(&sckConsumer);
        QString srcAddr = mtMsg.srcAddr;

        int nSleep = qrand() % 100;
        Sleep(nSleep); // 处理任务

//        qDebug() << QStringLiteral("接收任务") << mtMsg.toString();
        QString sSrcInfoSeq = QString(mtMsg.info).split(" ").last(); //记录测试client的任务序号

        // 发送询问消息
        int nAimSum = mtMsg.total;
        int nYetSum = 1;
        int nOffset = 1;
        int nCredit = (nAimSum-1) > c_PipeLine ?  c_PipeLine : (nAimSum-1);
        while (nYetSum < nAimSum)
        {
            while (nCredit)
            {
                bool bRes = ZMDUtils::sendmore(&sckConsumer, ""); // 一个空帧，用于代理判断Worker的消息类型
                MTMessage msSend = {mtQuery, uUID, srcAddr, mtMsg.total, nOffset++, infString,
                                    QString("send queryInfo to %1 from %2 for %3")
                                    .arg(ZMDUtils::lastAddr(srcAddr)).arg(ZMDUtils::lastAddr(uUID)).arg(sSrcInfoSeq).toLocal8Bit()};
                bRes = bRes && ZMDUtils::sendMsg(&sckConsumer, msSend);
//                qDebug() << QStringLiteral("发出询问") << ZMDCommon::lastAddr(uUID) << ZMDCommon::lastAddr(srcAddr) << mtMsg.info << bRes;
                nCredit--;
            }

            if (nOffset < nAimSum)
            {
                nCredit++;
            }

            //接收询问返回消息
            MTMessage msQuery = ZMDUtils::resvMsg(&sckConsumer);
            srcAddr = msQuery.srcAddr;
            nYetSum++;
            qDebug() << QStringLiteral("返回询问") << ZMDUtils::lastAddr(msQuery.dstAddr) << ZMDUtils::lastAddr(msQuery.srcAddr) << msQuery.info;
        }

        // 发送处理结果
        bool bRes = ZMDUtils::sendmore(&sckConsumer, ""); // 一个空帧，用于代理判断Worker的消息类型
        MTMessage msSend = {mtResult, uUID, srcAddr, 1, 0, infString, QString("send result to %1 from %2 for %3")
                            .arg(ZMDUtils::lastAddr(srcAddr)).arg(ZMDUtils::lastAddr(uUID)).arg(sSrcInfoSeq).toLocal8Bit()};
        bRes = bRes && ZMDUtils::sendMsg(&sckConsumer, msSend);
        qDebug() << QStringLiteral("发送结果") << msSend.toString() << bRes;
        sckConsumer.close();
    }

    context.close();
}




int main(int argc, char *argv[])
{
    qDebug() << QStringLiteral("请输入启动ip地址");
    QCoreApplication a(argc, argv);

//    std::string str1, str2, str3;
//    qDebug() << QStringLiteral("请输入启动ip地址");
//    std::cin >> str1;
//    qDebug() << QStringLiteral("请输入Worker数目");
//    std::cin >> str2;
//    qDebug() << QStringLiteral("请输入Task数目");
//    std::cin >> str3;


//    g_IP = QString::fromStdString(str1);
//    g_WorkerCnt = QString::fromStdString(str2).toInt();
//    g_TaskCnt = QString::fromStdString(str3).toInt();

//    QtConcurrent::run(createRouterProxy);

//    for (int i = 0; i < g_WorkerCnt; i++)
//    {
//        QString uUID = QUuid::createUuid().toString();
//        QtConcurrent::run(createWorker, uUID);
//    }

//    QString uUID = QUuid::createUuid().toString();
//    QtConcurrent::run(createClient, uUID, g_TaskCnt);

    return a.exec();
}


