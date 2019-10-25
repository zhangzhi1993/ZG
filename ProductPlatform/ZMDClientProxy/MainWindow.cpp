#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QByteArray>
#include <QHostAddress>
#include <QNetworkAddressEntry>
#include <QDebug>
#include <QQueue>
#include <QUuid>
#include <QString>
#include <QDateTime>
#include <QVBoxLayout>
#include <QUdpSocket>
#include <QPushButton>
#include <iostream>
#include "czmq.h"
#include "MTMessage.h"
#include "zhelpers.h"
#include "ZMDConst.h"

const int c_Retry = 10;
const int c_BroadCastLoop = 1000;
const int c_RouterProxyLoop = 5;
const int c_TimeHeartBit = 1000 * 2;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    aButton(NULL),
    m_TaskCnt(10),
    m_multiTaskCnt(1)
{
    ui->setupUi(this);
    us = new QUdpSocket;

    QWidget *centerWindow = new QWidget;
    this->setCentralWidget(centerWindow);

    QVBoxLayout *aLayout = new QVBoxLayout;
    aButton = new QPushButton();
    aButton->setText(QStringLiteral("һ��������"));
    aLayout->addWidget(aButton);
    centerWindow->setLayout(aLayout);
    connect(aButton, SIGNAL(clicked(bool)), this, SLOT(createClient()));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete us;
    delete aButton;
}

void MainWindow::setTaskCnt(int taskCnt, int multiTaskCnt)
{
    m_TaskCnt = taskCnt;
    m_multiTaskCnt = multiTaskCnt;
}


void MainWindow::doBroadCast(QList<int> sPortUDPBroadCasts)
{
//    qDebug() << QStringLiteral("�ظ��㲥");
    /* �����͹㲥�������ڵڶ������� */
    QList<QNetworkInterface> networkinterfaces = QNetworkInterface::allInterfaces();

    foreach (QNetworkInterface interface, networkinterfaces)
    {
        foreach (QNetworkAddressEntry entry, interface.addressEntries())
        {
            QHostAddress broadcastAddress = entry.broadcast();
            if (broadcastAddress != QHostAddress::Null
                && entry.ip() != QHostAddress::LocalHost
                && entry.ip().protocol() == QAbstractSocket::IPv4Protocol
                )
            {
                QString ba = QString("%1%2%3").arg(entry.ip().toString())
                        .arg(c_Separator).arg(QUuid::createUuid().toString());
//                qDebug() << ba << sPortUDPBroadCasts;
                foreach (int nPortUDPBroadCast, sPortUDPBroadCasts) {
                    us->writeDatagram(ba.toStdString().data(), broadcastAddress, nPortUDPBroadCast);  // UDP ��������
                }
            }
        }
     }
}

void MainWindow::createClient()
{
    QString uUID = QUuid::createUuid().toString();
    QString uRID = ZMDUtils::refreshAddr(uUID);
    std::string sUID = uUID.toStdString();
    std::string sRID = uRID.toStdString();
    qDebug() << "createClient" << uUID << uRID;

    // ���ι㲥��Ϣ
    QTime lastBroadCast = QTime::currentTime().addMSecs(c_BroadCastLoop);
    doBroadCast(cWorkerPortUDPBroadCasts);

    // �󶨶˿�
    zmq::context_t context(1);
    zmq::socket_t pBackSocket (context, ZMQ_ROUTER);
    pBackSocket.bind(QString("tcp://*:%1").arg(cPortServer_Proxy).toStdString());//��˰�;
    zmq::socket_t pHeartSocket (context, ZMQ_ROUTER);
    pHeartSocket.bind(QString("tcp://*:%1").arg(cPortServer_Proxy2).toStdString());//��˰�;
    zmq_pollitem_t items [] = {
        { pBackSocket, 0, ZMQ_POLLIN, 0 },
        { pHeartSocket, 0, ZMQ_POLLIN, 0 }
    };

    // ����
    int nSendSeq = 0;
    QMap<int, MTMessage> mapTasks; //��Ҫ���͵�����
    QMap<QString, QPair<QTime, MTMessage> > mapHearts; //�Ѿ����͵�������ϴ�����ʱ��
    QQueue<MTMessage> lstTasks;
    // ��һ�η��͵���Ϣ
    for (int i = 0; i < m_TaskCnt-m_multiTaskCnt; i++)
    {
        ++nSendSeq;
        MTMessage msSend = {mtTask, QUuid::createUuid().toString(), uRID, nSendSeq, "", 0, 1, 0, infString, QString("send work from %1 for info:%2")
                            .arg(ZMDUtils::lastAddr(uUID)).arg(nSendSeq).toLocal8Bit()}; //ע�������srcAddr������sckExchanger��sRID
        mapTasks.insert(nSendSeq, msSend);
        lstTasks.enqueue(msSend);
    }
    // ��Ҫ��η��͵���Ϣ�����ȷ���һ��Head��Ϣ�����շ�����Ϣ��ȷ��Worker�ĵ�ַ��Ȼ��Է�ͨ��Query����ѯ֮�����Ϣ
    QByteArray sinfo;
    for (int i = 0; i < m_multiTaskCnt; i++)
    {
        nSendSeq++;
        int nChunckCnt = 10;
        ZMDUtils::readFileString("MicrosoftOfficeProfessionalPlus2007.rar", sinfo);
        MTMessage msSend = {mtTask, QUuid::createUuid().toString(), uRID, nSendSeq, "", 0, nChunckCnt, 0, infFile, sinfo};
        mapTasks.insert(nSendSeq, msSend);
        lstTasks.enqueue(msSend);
    }

    qDebug() << QStringLiteral("��ʼѰ��Worker��������") << QTime::currentTime().toString();
    // ��Ϣ����
    QQueue<QString> sWorkersList;
    int nResultCnt = 0;
    while (true)
    {
        // �ظ��㲥��Ϣ
        if (QTime::currentTime() >= lastBroadCast)
        {
            doBroadCast(cWorkerPortUDPBroadCasts);
            lastBroadCast = QTime::currentTime().addMSecs(c_BroadCastLoop);
        }

        zmq_poll (items, 1, c_RouterProxyLoop);
        if (items [0].revents & ZMQ_POLLIN) // �Թ�������Ŷ������ڸ��ؾ��⣬��ת����˵�ǰ�˵���Ϣ
        {
            // �ŷ�ǰ׺��ʽ
            QString srcAddr = ZMDUtils::resv(&pBackSocket);

            // ����֡��readyָ������Ϣ
            QString sInfo = ZMDUtils::resv(&pBackSocket);
            QString sReady = sInfo.split(c_Separator).first();
            QString sHeartAddr = sInfo.split(c_Separator).last();
            if (sReady == c_Ready)
            {
                sWorkersList.enqueue(srcAddr);
            }
            else if (sReady == c_HeartBit)
            {
                if (mapHearts.contains(srcAddr))
                {
                    QPair<QTime, MTMessage> aHeart = mapHearts.value(srcAddr);
                    aHeart.first = QTime::currentTime().addMSecs(c_TimeHeartBit);
                    mapHearts.insert(srcAddr, aHeart);
                }
                ZMDUtils::sendmore(&pHeartSocket, sHeartAddr);
                ZMDUtils::send(&pHeartSocket, c_HeartBit);
            }
            else if (sReady == c_UnUsed)
            {
//                qDebug() << QStringLiteral("����ɾ����ʱWorker") << srcAddr;
                if (sWorkersList.removeOne(srcAddr)) //����ȷ��ɾ�����ź�
                {
                    // ��װ�ŷ�
                    ZMDUtils::sendmore(&pBackSocket, srcAddr);
                    ZMDUtils::send(&pBackSocket, c_Delete);
                }
                else
                {
                    ZMDUtils::sendmore(&pHeartSocket, sHeartAddr);
                    ZMDUtils::send(&pHeartSocket, c_HeartBit);
                }

            }
            else //ת����˵�ǰ�˵���Ϣ
            {
                MTMessage mtMsg = ZMDUtils::resvMsg(&pBackSocket);
    //            qDebug() << QStringLiteral("������Ϣ") << mtMsg.toString();
                QString srcAddr = mtMsg.srcAddr;
                switch (mtMsg.mtType) {
                case mtResult:
                    nResultCnt++;
                    if (mapTasks.keys().contains(mtMsg.ackNo))
                    {
                        qDebug() << QStringLiteral("���ս��") << ZMDUtils::lastAddr(uRID) << ZMDUtils::lastAddr(srcAddr) << mtMsg.info ;
                        mapTasks.remove(mtMsg.ackNo);
                        mapHearts.remove(mtMsg.srcAddr);
                    }
                    break;
                case mtQuery:
                    {
                        ++nSendSeq;
                        qDebug() << QStringLiteral("����ѯ��") << ZMDUtils::lastAddr(uRID) << ZMDUtils::lastAddr(srcAddr) << mtMsg.info << mtMsg.sequence;
                        int nSleep = qrand() % 100;
                        Sleep(nSleep); //!!!! ��ѯ�µ�����
                        MTMessage msSend = {mtResult, QUuid::createUuid().toString(), uRID, nSendSeq, srcAddr, 0, mtMsg.total, mtMsg.sequence, infString,
                                            QString("send queryRes from %1 to %2 for info:%3_%4")
                                            .arg(ZMDUtils::lastAddr(uRID)).arg(ZMDUtils::lastAddr(srcAddr)).arg(nSendSeq)
                                            .arg(mtMsg.sequence).toLocal8Bit()};

                        ZMDUtils::sendmore(&pBackSocket, srcAddr);
                        ZMDUtils::sendmore(&pBackSocket, "");
                        ZMDUtils::sendMsg(&pBackSocket, msSend);
                    }
                    break;
                default:
                    break;
                }
            }
        }
        if (!sWorkersList.isEmpty() && !lstTasks.isEmpty())
        {
            QString dstAddr = sWorkersList.dequeue();
            MTMessage msSend = lstTasks.dequeue();
            msSend.dstAddr = dstAddr;
            // ��װ�ŷ�
            ZMDUtils::sendmore(&pBackSocket, dstAddr);
            ZMDUtils::sendmore(&pBackSocket, "");
            ZMDUtils::sendMsg(&pBackSocket, msSend);
            mapHearts.insert(dstAddr, qMakePair(QTime::currentTime().addMSecs(c_TimeHeartBit), msSend));
            qDebug() << QStringLiteral("����һ���������") << msSend.toString() << QTime::currentTime().toString();;
        }
        if (mapTasks.isEmpty())
        {
            qDebug() << QStringLiteral("�������");
            break;
        }
        reOrgSendQQueue(lstTasks, mapHearts);
    }

    qDebug() << QStringLiteral("��������");
    pBackSocket.close();
    pHeartSocket.close();
    context.close();
}

void MainWindow::reOrgSendQQueue(QQueue<MTMessage> &lstTasks, QMap<QString, QPair<QTime, MTMessage> > &mapTasks)
{
    QList<QString> mapKeys = mapTasks.keys();
    for (int i = 0; i < mapKeys.count(); i++)
    {
        if (mapTasks.value(mapKeys.at(i)).first < QTime::currentTime())
        {
            if (!lstTasks.contains(mapTasks.value(mapKeys.at(i)).second))
            {
                qDebug() << QStringLiteral("��ʱ����") << mapKeys.at(i) << QTime::currentTime();
                lstTasks.enqueue(mapTasks.value(mapKeys.at(i)).second);
                QPair<QTime, MTMessage> aHeart = mapTasks.value(mapKeys.at(i));
                aHeart.first = QTime::currentTime().addMSecs(c_TimeHeartBit);
                mapTasks.insert(mapKeys.at(i), aHeart);
            }

        }
    }
}
