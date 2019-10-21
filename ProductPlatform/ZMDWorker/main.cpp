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
/// ���ɴ�����Client��Server֮��Ľ��������ֶ�ʵ�ָ���ƽ��

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "Advapi32.lib")

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
        // ��������
        zmq::socket_t sckConsumer (context, ZMQ_DEALER);
        sckConsumer.setsockopt(ZMQ_IDENTITY, sUID.data(), sUID.size());
        sckConsumer.connect(QString("tcp://%1:%2").arg(g_IP).arg(cPortServer_Proxy).toStdString());

//        qDebug() << "createWorker" << uUID << ++nCnt;

        // ���ߴ����Ѿ�׼������
        ZMDUtils::send(&sckConsumer, c_Ready);

        // ��ȡ����
        MTMessage mtMsg = ZMDUtils::resvMsg(&sckConsumer);
        QString srcAddr = mtMsg.srcAddr;

        int nSleep = qrand() % 100;
        Sleep(nSleep); // ��������

//        qDebug() << QStringLiteral("��������") << mtMsg.toString();
        QString sSrcInfoSeq = QString(mtMsg.info).split(" ").last(); //��¼����client���������

        // ����ѯ����Ϣ
        int nAimSum = mtMsg.total;
        int nYetSum = 1;
        int nOffset = 1;
        int nCredit = (nAimSum-1) > c_PipeLine ?  c_PipeLine : (nAimSum-1);
        while (nYetSum < nAimSum)
        {
            while (nCredit)
            {
                bool bRes = ZMDUtils::sendmore(&sckConsumer, ""); // һ����֡�����ڴ����ж�Worker����Ϣ����
                MTMessage msSend = {mtQuery, uUID, srcAddr, mtMsg.total, nOffset++, infString,
                                    QString("send queryInfo to %1 from %2 for %3")
                                    .arg(ZMDUtils::lastAddr(srcAddr)).arg(ZMDUtils::lastAddr(uUID)).arg(sSrcInfoSeq).toLocal8Bit()};
                bRes = bRes && ZMDUtils::sendMsg(&sckConsumer, msSend);
//                qDebug() << QStringLiteral("����ѯ��") << ZMDCommon::lastAddr(uUID) << ZMDCommon::lastAddr(srcAddr) << mtMsg.info << bRes;
                nCredit--;
            }

            if (nOffset < nAimSum)
            {
                nCredit++;
            }

            //����ѯ�ʷ�����Ϣ
            MTMessage msQuery = ZMDUtils::resvMsg(&sckConsumer);
            srcAddr = msQuery.srcAddr;
            nYetSum++;
            qDebug() << QStringLiteral("����ѯ��") << ZMDUtils::lastAddr(msQuery.dstAddr) << ZMDUtils::lastAddr(msQuery.srcAddr) << msQuery.info;
        }

        // ���ʹ�����
        bool bRes = ZMDUtils::sendmore(&sckConsumer, ""); // һ����֡�����ڴ����ж�Worker����Ϣ����
        MTMessage msSend = {mtResult, uUID, srcAddr, 1, 0, infString, QString("send result to %1 from %2 for %3")
                            .arg(ZMDUtils::lastAddr(srcAddr)).arg(ZMDUtils::lastAddr(uUID)).arg(sSrcInfoSeq).toLocal8Bit()};
        bRes = bRes && ZMDUtils::sendMsg(&sckConsumer, msSend);
        qDebug() << QStringLiteral("���ͽ��") << msSend.toString() << bRes;
        sckConsumer.close();
    }

    context.close();
}

void createClient(QString uUID, int nTaskCnt)
{
    QString uRID = ZMDUtils::refreshAddr(uUID);
    std::string sUID = uUID.toStdString();
    std::string sRID = uRID.toStdString();
    qDebug() << "createClient" << uUID << uRID;

    zmq::context_t context(1);

    // �������ߣ�����client����������������server��ÿ�뷢��һ������ͬʱ�ռ��ʹ�ӡӦ����Ϣ
    zmq::socket_t sckProducer(context, ZMQ_DEALER);
//    ZMDCommon::setHighWaterLevel(&sckProducer, 1);
    sckProducer.setsockopt(ZMQ_IDENTITY, sUID.data(), sUID.size());
    sckProducer.connect(QString("tcp://%1:%2").arg(g_IP).arg(cPortClient_Proxy).toStdString());

    // ���ں�Worker������Ϣ
    zmq::socket_t sckExchanger(context, ZMQ_DEALER);
//    ZMDCommon::setHighWaterLevel(&sckExchanger, 1);
    sckExchanger.setsockopt(ZMQ_IDENTITY, sRID.data(), sRID.size());
    sckExchanger.connect(QString("tcp://%1:%2").arg(g_IP).arg(cPortClient_Server).toStdString());

    ZMDUtils::printHighWaterLevel(&sckProducer);
    ZMDUtils::printHighWaterLevel(&sckExchanger);

    zmq_pollitem_t items [] = {
        { sckProducer,  0, ZMQ_POLLIN, 0 },
        { sckExchanger, 0, ZMQ_POLLIN, 0 }
    };

    int nSendSeq = 0;

    // ��һ�η��͵���Ϣ
    for (int i = 0; i < nTaskCnt-g_multiTaskCnt; i++)
    {
        // ���������������
        ZMDUtils::sendmore(&sckProducer, "");
        MTMessage msSend = {mtTask, uRID, "", 1, 0, infString, QString("send work from %1 for info:%2")
                            .arg(ZMDUtils::lastAddr(uUID)).arg(++nSendSeq).toLocal8Bit()}; //ע�������srcAddr������sckExchanger��sRID
        bool bRes = ZMDUtils::sendMsg(&sckProducer, msSend);
//        qDebug() << QStringLiteral("��������") << msSend.toString() << bRes;
    }

    // ��Ҫ��η��͵���Ϣ�����ȷ���һ��Head��Ϣ�����շ�����Ϣ��ȷ��Worker�ĵ�ַ��Ȼ��Է�ͨ��Query����ѯ֮�����Ϣ
    for (int i = 0; i < g_multiTaskCnt; i++)
    {
        nSendSeq++;
        int nChunckCnt = 10;
        ZMDUtils::sendmore(&sckProducer, "");
        MTMessage msSend = {mtTask, uRID, "", nChunckCnt, 0, infString, QString("send work from %1 for info:%2")
                            .arg(ZMDUtils::lastAddr(uUID)).arg(nSendSeq).toLocal8Bit()};
        ZMDUtils::sendMsg(&sckProducer, msSend);
    }


    int nResultCnt = 0;
    while (true)
    {
        zmq_poll (items, 2, -1);
        if (items [0].revents & ZMQ_POLLIN) // �������յ���Ϣ
        {
            qDebug() << QStringLiteral("%1�����˴���").arg(ZMDUtils::lastAddr(uUID));
        }
        if (items [1].revents & ZMQ_POLLIN)
        {
            ZMDUtils::resv(&sckExchanger); // ��һ֡�ǿ�
            MTMessage mtMsg = ZMDUtils::resvMsg(&sckExchanger);
//            qDebug() << QStringLiteral("������Ϣ") << mtMsg.toString();
            QString srcAddr = mtMsg.srcAddr;
            switch (mtMsg.mtType) {
            case mtResult:
                nResultCnt++;
                qDebug() << QStringLiteral("���ս��") << ZMDUtils::lastAddr(uRID) << ZMDUtils::lastAddr(srcAddr) << mtMsg.info ;
                break;
            case mtQuery:
                {
                    qDebug() << QStringLiteral("����ѯ��") << ZMDUtils::lastAddr(uRID) << ZMDUtils::lastAddr(srcAddr) << mtMsg.info << mtMsg.sequence;
                    MTMessage msSend = {mtResult, uRID, srcAddr, mtMsg.total, mtMsg.sequence, infString,
                                        QString("send queryRes from %1 to %2 for info:%3_%4")
                                        .arg(ZMDUtils::lastAddr(uRID)).arg(ZMDUtils::lastAddr(srcAddr)).arg(++nSendSeq)
                                        .arg(mtMsg.sequence).toLocal8Bit()};
                    ZMDUtils::sendmore(&sckExchanger, "");
                    ZMDUtils::sendMsg(&sckExchanger, msSend);
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

    qDebug() << QStringLiteral("�������");
    sckProducer.close();
    sckExchanger.close();
    context.close();
}


int main(int argc, char *argv[])
{
    qDebug() << QStringLiteral("����������ip��ַ");
    QCoreApplication a(argc, argv);

//    std::string str1, str2, str3;
//    qDebug() << QStringLiteral("����������ip��ַ");
//    std::cin >> str1;
//    qDebug() << QStringLiteral("������Worker��Ŀ");
//    std::cin >> str2;
//    qDebug() << QStringLiteral("������Task��Ŀ");
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


