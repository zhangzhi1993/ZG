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
        // ��������
        zmq::socket_t sckConsumer (context, ZMQ_DEALER);
        sckConsumer.setsockopt(ZMQ_IDENTITY, sUID.data(), sUID.size());
        sckConsumer.connect(QString("tcp://%1:%2").arg(g_IP).arg(cPortServer_Proxy).toStdString());

//        qDebug() << "createWorker" << uUID << ++nCnt;

        // ���ߴ����Ѿ�׼������
        ZeroUtils::send(&sckConsumer, c_Ready);

        // ��ȡ����
        MTMessage mtMsg = ZeroUtils::resvMsg(&sckConsumer);
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
                bool bRes = ZeroUtils::sendmore(&sckConsumer, ""); // һ����֡�����ڴ����ж�Worker����Ϣ����
                MTMessage msSend = {mtQuery, uUID, srcAddr, mtMsg.total, nOffset++, infString,
                                    QString("send queryInfo to %1 from %2 for %3")
                                    .arg(ZeroUtils::lastAddr(srcAddr)).arg(ZeroUtils::lastAddr(uUID)).arg(sSrcInfoSeq).toLocal8Bit()};
                bRes = bRes && ZeroUtils::sendMsg(&sckConsumer, msSend);
//                qDebug() << QStringLiteral("����ѯ��") << ZeroUtils::lastAddr(uUID) << ZeroUtils::lastAddr(srcAddr) << mtMsg.info << bRes;
                nCredit--;
            }

            if (nOffset < nAimSum)
            {
                nCredit++;
            }

            //����ѯ�ʷ�����Ϣ
            MTMessage msQuery = ZeroUtils::resvMsg(&sckConsumer);
            srcAddr = msQuery.srcAddr;
            nYetSum++;
            qDebug() << QStringLiteral("����ѯ��") << ZeroUtils::lastAddr(msQuery.dstAddr) << ZeroUtils::lastAddr(msQuery.srcAddr) << msQuery.info;
        }

        // ���ʹ�����
        bool bRes = ZeroUtils::sendmore(&sckConsumer, ""); // һ����֡�����ڴ����ж�Worker����Ϣ����
        MTMessage msSend = {mtResult, uUID, srcAddr, 1, 0, infString, QString("send result to %1 from %2 for %3")
                            .arg(ZeroUtils::lastAddr(srcAddr)).arg(ZeroUtils::lastAddr(uUID)).arg(sSrcInfoSeq).toLocal8Bit()};
        bRes = bRes && ZeroUtils::sendMsg(&sckConsumer, msSend);
        qDebug() << QStringLiteral("���ͽ��") << msSend.toString() << bRes;
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

    // �������ߣ�����client����������������server��ÿ�뷢��һ������ͬʱ�ռ��ʹ�ӡӦ����Ϣ
    zmq::socket_t sckProducer(context, ZMQ_DEALER);
//    ZeroUtils::setHighWaterLevel(&sckProducer, 1);
    sckProducer.setsockopt(ZMQ_IDENTITY, sUID.data(), sUID.size());
    sckProducer.connect(QString("tcp://%1:%2").arg(g_IP).arg(cPortClient_Proxy).toStdString());

    // ���ں�Worker������Ϣ
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

    // ��һ�η��͵���Ϣ
    for (int i = 0; i < nTaskCnt-g_multiTaskCnt; i++)
    {
        // ���������������
        ZeroUtils::sendmore(&sckProducer, "");
        MTMessage msSend = {mtTask, uRID, "", 1, 0, infString, QString("send work from %1 for info:%2")
                            .arg(ZeroUtils::lastAddr(uUID)).arg(++nSendSeq).toLocal8Bit()}; //ע�������srcAddr������sckExchanger��sRID
        bool bRes = ZeroUtils::sendMsg(&sckProducer, msSend);
//        qDebug() << QStringLiteral("��������") << msSend.toString() << bRes;
    }

    // ��Ҫ��η��͵���Ϣ�����ȷ���һ��Head��Ϣ�����շ�����Ϣ��ȷ��Worker�ĵ�ַ��Ȼ��Է�ͨ��Query����ѯ֮�����Ϣ
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
        if (items [0].revents & ZMQ_POLLIN) // �������յ���Ϣ
        {
            qDebug() << QStringLiteral("%1�����˴���").arg(ZeroUtils::lastAddr(uUID));
        }
        if (items [1].revents & ZMQ_POLLIN)
        {
            ZeroUtils::resv(&sckExchanger); // ��һ֡�ǿ�
            MTMessage mtMsg = ZeroUtils::resvMsg(&sckExchanger);
//            qDebug() << QStringLiteral("������Ϣ") << mtMsg.toString();
            QString srcAddr = mtMsg.srcAddr;
            switch (mtMsg.mtType) {
            case mtResult:
                nResultCnt++;
                qDebug() << QStringLiteral("���ս��") << ZeroUtils::lastAddr(uRID) << ZeroUtils::lastAddr(srcAddr) << mtMsg.info ;
                break;
            case mtQuery:
                {
                    qDebug() << QStringLiteral("����ѯ��") << ZeroUtils::lastAddr(uRID) << ZeroUtils::lastAddr(srcAddr) << mtMsg.info << mtMsg.sequence;
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

    qDebug() << QStringLiteral("�������");
    sckProducer.close();
    sckExchanger.close();
    context.close();
}


void createRouterProxy()
{
    zmq::context_t context(1);

    zmq::socket_t pFrontSocket1 (context, ZMQ_ROUTER);
//    ZeroUtils::setHighWaterLevel(&pFrontSocket1, 1);
    pFrontSocket1.bind(QString("tcp://*:%1").arg(cPortClient_Server).toStdString()); //����ָ����Client������Ϣ��ָ����Worker

    zmq::socket_t pFrontSocket2 (context, ZMQ_ROUTER);
//    ZeroUtils::setHighWaterLevel(&pFrontSocket2, 1);
    pFrontSocket2.bind(QString("tcp://*:%1").arg(cPortClient_Proxy).toStdString()); //ǰ�˰�;ֻ���ڷ������񣬼���ʱClient��֪���ú��ĸ�Worker��ϵ

    zmq::socket_t pBackSocket (context, ZMQ_ROUTER);
//    ZeroUtils::setHighWaterLevel(&pBackSocket, 1);
    pBackSocket.bind(QString("tcp://*:%1").arg(cPortServer_Proxy).toStdString());//��˰�;

    ZeroUtils::printHighWaterLevel(&pFrontSocket1);
    ZeroUtils::printHighWaterLevel(&pFrontSocket2);
    ZeroUtils::printHighWaterLevel(&pBackSocket);

    //  ��frontend��backend�������Ϣ
    zmq_pollitem_t items [] = {
        { pBackSocket, 0, ZMQ_POLLIN, 0 },
        { pFrontSocket1,  0, ZMQ_POLLIN, 0 },
        { pFrontSocket2,  0, ZMQ_POLLIN, 0 }
    };

    QQueue<QString> sWorkersList;
    int nLastWorkersCnt = 0;

    while (true)
    {
        zmq_poll (items, 3, -1); //��������

        if (items [0].revents & ZMQ_POLLIN) // �Թ�������Ŷ������ڸ��ؾ��⣬��ת����˵�ǰ�˵���Ϣ
        {
            // �ŷ�ǰ׺��ʽ
            QString srcAddr = ZeroUtils::resv(&pBackSocket);

            // ����֡��readyָ������Ϣ
            QString sReady = ZeroUtils::resv(&pBackSocket);
            if (sReady != c_Ready) //ת����˵�ǰ�˵���Ϣ
            {
                MTMessage msResv = ZeroUtils::resvMsg(&pBackSocket);
                QString dstAddr = msResv.dstAddr;

//                qDebug() << "pBackSocket" << msResv.toString();
                // ��װ�ŷ�
                ZeroUtils::sendmore(&pFrontSocket1, dstAddr);
                ZeroUtils::sendmore(&pFrontSocket1, "");
                ZeroUtils::sendMsg(&pFrontSocket1, msResv);
            }
            else
            {
                sWorkersList.enqueue(srcAddr);
            }
        }
        if (items [1].revents & ZMQ_POLLIN) // ת��ǰ�˵���˵���Ϣ
        {
            // �ŷ�ǰ׺��ʽ
            QString srcAddr = ZeroUtils::resv(&pFrontSocket1);
            ZeroUtils::resv(&pFrontSocket1); //�ڶ�֡�ǿ�

             // ָ����Client��ָ����Worker��ϵ��Ŀ���ַ��Ϊ��
            MTMessage msResv = ZeroUtils::resvMsg(&pFrontSocket1);

            QString dstAddr = msResv.dstAddr;
            // ��װ�ŷ�
            ZeroUtils::sendmore(&pBackSocket, dstAddr);
            ZeroUtils::sendMsg(&pBackSocket, msResv);
        }
        if (items [2].revents & ZMQ_POLLIN && !sWorkersList.isEmpty()) // ���ڿͻ��˷�������
        {
            // �ŷ�ǰ׺��ʽ
            QString srcAddr = ZeroUtils::resv(&pFrontSocket2);
            ZeroUtils::resv(&pFrontSocket2); //�ڶ�֡�ǿ�

             // �����Ľ�����Ϣ�����������������Ϣ��Ŀ���ַΪ��
            MTMessage msResv = ZeroUtils::resvMsg(&pFrontSocket2);
            QString dstAddr = sWorkersList.dequeue();
            msResv.dstAddr = dstAddr;
//            qDebug() << "pFrontSocket" << msResv.toString();
            // ��װ�ŷ�
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
    qDebug() << QStringLiteral("����������ip��ַ");
    std::cin >> str1;
    qDebug() << QStringLiteral("������Worker��Ŀ");
    std::cin >> str2;
    qDebug() << QStringLiteral("������Task��Ŀ");
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


