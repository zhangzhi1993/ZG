#include "ZeroUtils.h"
#include <QFile>
#include <QDataStream>
#include <QUuid>
#include <QDebug>
#include <QQueue>

const int cPortClient_Server = 6001;
const int cPortClient_Proxy = 6003;
const int cPortServer_Proxy = 6004;

const QString c_Ready = "Ready";
int g_multiTaskCnt = 2;
const int c_PipeLine = 5;

ZeroUtils::ZeroUtils()
{

}

QString ZeroUtils::resv(zmq::socket_t *socket)
{
    zmq::message_t request;
    socket->recv(&request);

    char *recvData = (char *)malloc(request.size() + 1);
    memcpy(recvData, request.data(), request.size());
    recvData[request.size()] = '\0';
    QString sMsg = QString(recvData);
    free(recvData);
    return sMsg;
}

MTMessage ZeroUtils::resvMsg(zmq::socket_t *socket)
{
    MSGTYPE mtType = (MSGTYPE)ZeroUtils::resv(socket).toInt();
    QString sSrcAddr = ZeroUtils::resv(socket);
    QString sdstAddr = ZeroUtils::resv(socket);
    int nTotal = ZeroUtils::resv(socket).toInt();
    int nSequence = ZeroUtils::resv(socket).toInt();
    INFOTYPE infType = (INFOTYPE)ZeroUtils::resv(socket).toInt();
    QByteArray sMsg = ZeroUtils::resvByteArr(socket);
    MTMessage mtMsg = {mtType, sSrcAddr, sdstAddr, nTotal, nSequence, infType, sMsg};
    return mtMsg;
}

bool ZeroUtils::sendMsg(zmq::socket_t *socket, MTMessage mtMsg)
{
    bool bRes = ZeroUtils::sendmore(socket, QString::number(mtMsg.mtType));
    bRes = bRes && ZeroUtils::sendmore(socket, mtMsg.srcAddr);
    bRes = bRes && ZeroUtils::sendmore(socket, mtMsg.dstAddr);
    bRes = bRes && ZeroUtils::sendmore(socket, QString::number(mtMsg.total));
    bRes = bRes && ZeroUtils::sendmore(socket, QString::number(mtMsg.sequence));
    bRes = bRes && ZeroUtils::sendmore(socket, QString::number(mtMsg.infType));
    bRes = bRes && ZeroUtils::sendByteArr(socket, mtMsg.info);
    return bRes;
}

bool ZeroUtils::send(zmq::socket_t *socket, QString msg)
{
    std::string stdStr = msg.toStdString();
    zmq::message_t request(stdStr.size());
    memcpy(request.data(), stdStr.data(), stdStr.size());
    return socket->send(request);
}

bool ZeroUtils::sendmore(zmq::socket_t *socket, QString msg)
{
    std::string stdStr = msg.toStdString();
    zmq::message_t request(stdStr.size());
    memcpy(request.data(), stdStr.data(), stdStr.size());
    return socket->send(request, ZMQ_SNDMORE);
}

QString ZeroUtils::lastAddr(QString addr)
{
    return "{" + addr.right(5);
}

QString ZeroUtils::refreshAddr(QString addr)
{
    int nLen = addr.length();
    int nRef = 5;
    QString uUID = QUuid::createUuid().toString();
    return addr.left(nLen-nRef) + uUID.right(nRef);
}

void ZeroUtils::setHighWaterLevel(zmq::socket_t *socket, int highWaterLevel)
{
    size_t nSize = sizeof(int);
    socket->setsockopt(ZMQ_RCVHWM, &highWaterLevel, nSize);
    socket->setsockopt(ZMQ_SNDHWM, &highWaterLevel, nSize);
}

QByteArray ZeroUtils::resvByteArr(zmq::socket_t *socket)
{
    zmq::message_t request;
    socket->recv(&request);

    char *recvData = (char *)malloc(request.size());
    memcpy(recvData, request.data(), request.size());
    QByteArray sMsg(recvData, request.size());
    free(recvData);
    return sMsg;
}

bool ZeroUtils::sendByteArr(zmq::socket_t *socket, QByteArray msg)
{
    zmq::message_t request(msg.size());
    memcpy(request.data(), msg.data(), msg.size());
    return socket->send(request);
}

void ZeroUtils::printHighWaterLevel(zmq::socket_t *socket)
{
    int highRcvWaterLevel = 0;
    int highRndWaterLevel = 0;
    size_t nSize = sizeof(int);
    socket->getsockopt(ZMQ_RCVHWM, &highRcvWaterLevel, &nSize);
    socket->getsockopt(ZMQ_SNDHWM, &highRndWaterLevel, &nSize);
    qDebug() << "ZMQ_RCVHWM" << highRcvWaterLevel << "ZMQ_SNDHWM" << highRndWaterLevel;
}

bool ZeroUtils::readFileString(QString path, QByteArray &data)
{
    if(path.isEmpty() == false)
    {
        QFile file(path);
        bool ret = file.open(QIODevice::ReadOnly);
        if(ret == true)
        {
            //���ļ����ݶ���
            data = file.readAll();
            file.close();
            return true;
        }
    }
    return false;
}

bool ZeroUtils::writeFileString(QString path, const QByteArray &data)
{
    if(path.isEmpty() == false)
    {
        QFile file(path);
        bool ret = file.open(QIODevice::WriteOnly);
        if(ret == true)
        {
            //���ļ����ݶ���
            file.write(data, data.size());
            file.close();
            return true;
        }
    }
    return false;
}

QString MTMessage::toString()
{
    QString sType;
    switch (mtType) {
    case mtTask:
        sType = "mtTask";
        break;
    case mtResult:
        sType = "mtResult";
        break;
    case mtQuery:
        sType = "mtQuery";
        break;
    default:
        break;
    }
    return QString("%1 %2 %3 %4").arg(sType).arg(ZeroUtils::lastAddr(srcAddr))
            .arg(ZeroUtils::lastAddr(dstAddr)).arg(QString(info));
}

void CreateUtils::createRouterProxy()
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
    return;
}

void CreateUtils::createWorker(QString uUID, QString sIP)
{
    std::string sUID = uUID.toStdString();
    qDebug() << "createWorker" << uUID;

    zmq::context_t context(1);
    while (1)
    {
        // ��������
        zmq::socket_t sckConsumer (context, ZMQ_DEALER);
        sckConsumer.setsockopt(ZMQ_IDENTITY, sUID.data(), sUID.size());
        sckConsumer.connect(QString("tcp://%1:%2").arg(sIP).arg(cPortServer_Proxy).toStdString());

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

void CreateUtils::createClient(QString uUID, QString sIP, int nTaskCnt)
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
    sckProducer.connect(QString("tcp://%1:%2").arg(sIP).arg(cPortClient_Proxy).toStdString());

    // ���ں�Worker������Ϣ
    zmq::socket_t sckExchanger(context, ZMQ_DEALER);
//    ZeroUtils::setHighWaterLevel(&sckExchanger, 1);
    sckExchanger.setsockopt(ZMQ_IDENTITY, sRID.data(), sRID.size());
    sckExchanger.connect(QString("tcp://%1:%2").arg(sIP).arg(cPortClient_Server).toStdString());

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
