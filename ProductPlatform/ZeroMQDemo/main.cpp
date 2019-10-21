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

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "Advapi32.lib")

const int cPortClient_Server = 6001;
const int cPortClient_Proxy = 6003;
const int cPortServer_Proxy = 6004;

const QString c_Ready = "Ready";
QString g_IP = "localhost";
int g_WorkerCnt = 3;
int g_TaskCnt = 10;
int g_multiTaskCnt = 2;
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
        ZeroUtils::send(&sckConsumer, c_Ready);

        // 获取请求
        MTMessage mtMsg = ZeroUtils::resvMsg(&sckConsumer);
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
                bool bRes = ZeroUtils::sendmore(&sckConsumer, ""); // 一个空帧，用于代理判断Worker的消息类型
                MTMessage msSend = {mtQuery, uUID, srcAddr, mtMsg.total, nOffset++, infString,
                                    QString("send queryInfo to %1 from %2 for %3")
                                    .arg(ZeroUtils::lastAddr(srcAddr)).arg(ZeroUtils::lastAddr(uUID)).arg(sSrcInfoSeq).toLocal8Bit()};
                bRes = bRes && ZeroUtils::sendMsg(&sckConsumer, msSend);
//                qDebug() << QStringLiteral("发出询问") << ZeroUtils::lastAddr(uUID) << ZeroUtils::lastAddr(srcAddr) << mtMsg.info << bRes;
                nCredit--;
            }

            if (nOffset < nAimSum)
            {
                nCredit++;
            }

            //接收询问返回消息
            MTMessage msQuery = ZeroUtils::resvMsg(&sckConsumer);
            srcAddr = msQuery.srcAddr;
            nYetSum++;
            qDebug() << QStringLiteral("返回询问") << ZeroUtils::lastAddr(msQuery.dstAddr) << ZeroUtils::lastAddr(msQuery.srcAddr) << msQuery.info;
        }

        // 发送处理结果
        bool bRes = ZeroUtils::sendmore(&sckConsumer, ""); // 一个空帧，用于代理判断Worker的消息类型
        MTMessage msSend = {mtResult, uUID, srcAddr, 1, 0, infString, QString("send result to %1 from %2 for %3")
                            .arg(ZeroUtils::lastAddr(srcAddr)).arg(ZeroUtils::lastAddr(uUID)).arg(sSrcInfoSeq).toLocal8Bit()};
        bRes = bRes && ZeroUtils::sendMsg(&sckConsumer, msSend);
        qDebug() << QStringLiteral("发送结果") << msSend.toString() << bRes;
        sckConsumer.close();
    }

    context.close();
}

void createClient(QString uUID, int nTaskCnt)
{
    QString uRID = ZeroUtils::refreshAddr(uUID);
    std::string sUID = uUID.toStdString();
    std::string sRID = uRID.toStdString();
    qDebug() << "createClient" << uUID << uRID;

    zmq::context_t context(1);

    // 任务发起者：这是client端任务，它会连接至server，每秒发送一次请求，同时收集和打印应答消息
    zmq::socket_t sckProducer(context, ZMQ_DEALER);
//    ZeroUtils::setHighWaterLevel(&sckProducer, 1);
    sckProducer.setsockopt(ZMQ_IDENTITY, sUID.data(), sUID.size());
    sckProducer.connect(QString("tcp://%1:%2").arg(g_IP).arg(cPortClient_Proxy).toStdString());

    // 用于和Worker交互信息
    zmq::socket_t sckExchanger(context, ZMQ_DEALER);
//    ZeroUtils::setHighWaterLevel(&sckExchanger, 1);
    sckExchanger.setsockopt(ZMQ_IDENTITY, sRID.data(), sRID.size());
    sckExchanger.connect(QString("tcp://%1:%2").arg(g_IP).arg(cPortClient_Server).toStdString());

    ZeroUtils::printHighWaterLevel(&sckProducer);
    ZeroUtils::printHighWaterLevel(&sckExchanger);

    zmq_pollitem_t items [] = {
        { sckProducer,  0, ZMQ_POLLIN, 0 },
        { sckExchanger, 0, ZMQ_POLLIN, 0 }
    };

    int nSendSeq = 0;

    // 能一次发送的消息
    for (int i = 0; i < nTaskCnt-g_multiTaskCnt; i++)
    {
        // 发送任务给处理器
        ZeroUtils::sendmore(&sckProducer, "");
        MTMessage msSend = {mtTask, uRID, "", 1, 0, infString, QString("send work from %1 for info:%2")
                            .arg(ZeroUtils::lastAddr(uUID)).arg(++nSendSeq).toLocal8Bit()}; //注意这里的srcAddr传的是sckExchanger的sRID
        bool bRes = ZeroUtils::sendMsg(&sckProducer, msSend);
//        qDebug() << QStringLiteral("发送任务") << msSend.toString() << bRes;
    }

    // 需要多次发送的消息：首先发送一条Head信息并接收返回信息来确定Worker的地址，然后对方通过Query来查询之后的信息
    for (int i = 0; i < g_multiTaskCnt; i++)
    {
        nSendSeq++;
        int nChunckCnt = 10;
        ZeroUtils::sendmore(&sckProducer, "");
        MTMessage msSend = {mtTask, uRID, "", nChunckCnt, 0, infString, QString("send work from %1 for info:%2")
                            .arg(ZeroUtils::lastAddr(uUID)).arg(nSendSeq).toLocal8Bit()};
        ZeroUtils::sendMsg(&sckProducer, msSend);
    }


    int nResultCnt = 0;
    while (true)
    {
        zmq_poll (items, 2, -1);
        if (items [0].revents & ZMQ_POLLIN) // 不可能收到消息
        {
            qDebug() << QStringLiteral("%1出现了错误").arg(ZeroUtils::lastAddr(uUID));
        }
        if (items [1].revents & ZMQ_POLLIN)
        {
            ZeroUtils::resv(&sckExchanger); // 第一帧是空
            MTMessage mtMsg = ZeroUtils::resvMsg(&sckExchanger);
//            qDebug() << QStringLiteral("返回信息") << mtMsg.toString();
            QString srcAddr = mtMsg.srcAddr;
            switch (mtMsg.mtType) {
            case mtResult:
                nResultCnt++;
                qDebug() << QStringLiteral("接收结果") << ZeroUtils::lastAddr(uRID) << ZeroUtils::lastAddr(srcAddr) << mtMsg.info ;
                break;
            case mtQuery:
                {
                    qDebug() << QStringLiteral("接收询问") << ZeroUtils::lastAddr(uRID) << ZeroUtils::lastAddr(srcAddr) << mtMsg.info << mtMsg.sequence;
                    MTMessage msSend = {mtResult, uRID, srcAddr, mtMsg.total, mtMsg.sequence, infString,
                                        QString("send queryRes from %1 to %2 for info:%3_%4")
                                        .arg(ZeroUtils::lastAddr(uRID)).arg(ZeroUtils::lastAddr(srcAddr)).arg(++nSendSeq)
                                        .arg(mtMsg.sequence).toLocal8Bit()};
                    ZeroUtils::sendmore(&sckExchanger, "");
                    ZeroUtils::sendMsg(&sckExchanger, msSend);
                }
                break;
            default:
                break;
            }
        }
        if (nResultCnt == nTaskCnt)
        {
            break;
        }
    }

    qDebug() << QStringLiteral("完成运算");
    sckProducer.close();
    sckExchanger.close();
    context.close();
}


void createRouterProxy()
{
    zmq::context_t context(1);

    zmq::socket_t pFrontSocket1 (context, ZMQ_ROUTER);
//    ZeroUtils::setHighWaterLevel(&pFrontSocket1, 1);
    pFrontSocket1.bind(QString("tcp://*:%1").arg(cPortClient_Server).toStdString()); //用于指定的Client发送消息到指定的Worker

    zmq::socket_t pFrontSocket2 (context, ZMQ_ROUTER);
//    ZeroUtils::setHighWaterLevel(&pFrontSocket2, 1);
    pFrontSocket2.bind(QString("tcp://*:%1").arg(cPortClient_Proxy).toStdString()); //前端绑定;只用于发送任务，即此时Client不知道该和哪个Worker联系

    zmq::socket_t pBackSocket (context, ZMQ_ROUTER);
//    ZeroUtils::setHighWaterLevel(&pBackSocket, 1);
    pBackSocket.bind(QString("tcp://*:%1").arg(cPortServer_Proxy).toStdString());//后端绑定;

    ZeroUtils::printHighWaterLevel(&pFrontSocket1);
    ZeroUtils::printHighWaterLevel(&pFrontSocket2);
    ZeroUtils::printHighWaterLevel(&pBackSocket);

    //  在frontend和backend间搬运消息
    zmq_pollitem_t items [] = {
        { pBackSocket, 0, ZMQ_POLLIN, 0 },
        { pFrontSocket1,  0, ZMQ_POLLIN, 0 },
        { pFrontSocket2,  0, ZMQ_POLLIN, 0 }
    };

    QQueue<QString> sWorkersList;
    int nLastWorkersCnt = 0;

    while (true)
    {
        zmq_poll (items, 3, -1); //最好用这个

        if (items [0].revents & ZMQ_POLLIN) // 对工人身份排队以用于负载均衡，并转发后端到前端的消息
        {
            // 信封前缀格式
            QString srcAddr = ZeroUtils::resv(&pBackSocket);

            // 第三帧是ready指令或空消息
            QString sReady = ZeroUtils::resv(&pBackSocket);
            if (sReady != c_Ready) //转发后端到前端的消息
            {
                MTMessage msResv = ZeroUtils::resvMsg(&pBackSocket);
                QString dstAddr = msResv.dstAddr;

//                qDebug() << "pBackSocket" << msResv.toString();
                // 封装信封
                ZeroUtils::sendmore(&pFrontSocket1, dstAddr);
                ZeroUtils::sendmore(&pFrontSocket1, "");
                ZeroUtils::sendMsg(&pFrontSocket1, msResv);
            }
            else
            {
                sWorkersList.enqueue(srcAddr);
            }
        }
        if (items [1].revents & ZMQ_POLLIN) // 转发前端到后端的信息
        {
            // 信封前缀格式
            QString srcAddr = ZeroUtils::resv(&pFrontSocket1);
            ZeroUtils::resv(&pFrontSocket1); //第二帧是空

             // 指定的Client和指定的Worker联系，目标地址不为空
            MTMessage msResv = ZeroUtils::resvMsg(&pFrontSocket1);

            QString dstAddr = msResv.dstAddr;
            // 封装信封
            ZeroUtils::sendmore(&pBackSocket, dstAddr);
            ZeroUtils::sendMsg(&pBackSocket, msResv);
        }
        if (items [2].revents & ZMQ_POLLIN && !sWorkersList.isEmpty()) // 用于客户端发送任务
        {
            // 信封前缀格式
            QString srcAddr = ZeroUtils::resv(&pFrontSocket2);
            ZeroUtils::resv(&pFrontSocket2); //第二帧是空

             // 真正的接收信息，由于是任务发起的信息，目标地址为空
            MTMessage msResv = ZeroUtils::resvMsg(&pFrontSocket2);
            QString dstAddr = sWorkersList.dequeue();
            msResv.dstAddr = dstAddr;
//            qDebug() << "pFrontSocket" << msResv.toString();
            // 封装信封
            ZeroUtils::sendmore(&pBackSocket, dstAddr);
            ZeroUtils::sendMsg(&pBackSocket, msResv);
        }
        if (nLastWorkersCnt != sWorkersList.count())
        {
            nLastWorkersCnt = sWorkersList.count();
//            qDebug() << sWorkersList;
        }
    }

    pBackSocket.close();
    pFrontSocket1.close();
    pFrontSocket2.close();
    context.close();
}


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


    g_IP = QString::fromStdString(str1);
    g_WorkerCnt = QString::fromStdString(str2).toInt();
    g_TaskCnt = QString::fromStdString(str3).toInt();

    QtConcurrent::run(createRouterProxy);

    for (int i = 0; i < g_WorkerCnt; i++)
    {
        QString uUID = QUuid::createUuid().toString();
        QtConcurrent::run(createWorker, uUID);
    }

    QString uUID = QUuid::createUuid().toString();
    QtConcurrent::run(createClient, uUID, g_TaskCnt);

    return a.exec();
}


