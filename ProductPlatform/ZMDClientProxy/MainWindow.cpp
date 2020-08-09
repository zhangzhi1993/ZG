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
    aButton->setText(QStringLiteral("一个新任务"));
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
//    qDebug() << QStringLiteral("重复广播");
    /* 单播和广播的区别在第二个参数 */
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
                    us->writeDatagram(ba.toStdString().data(), broadcastAddress, nPortUDPBroadCast);  // UDP 发送数据
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

    // 初次广播消息
    QTime lastBroadCast = QTime::currentTime().addMSecs(c_BroadCastLoop);
    doBroadCast(cWorkerPortUDPBroadCasts);

    // 绑定端口
    zmq::context_t context(1);
    zmq::socket_t pBackSocket (context, ZMQ_ROUTER);
    pBackSocket.bind(QString("tcp://*:%1").arg(cPortServer_Proxy).toStdString());//后端绑定;
    zmq::socket_t pHeartSocket (context, ZMQ_ROUTER);
    pHeartSocket.bind(QString("tcp://*:%1").arg(cPortServer_Proxy2).toStdString());//后端绑定;
    zmq_pollitem_t items [] = {
        { pBackSocket, 0, ZMQ_POLLIN, 0 },
        { pHeartSocket, 0, ZMQ_POLLIN, 0 }
    };

    // 任务集
    int nSendSeq = 0;
    QMap<int, MTMessage> mapTasks; //需要发送的任务
    QMap<QString, QPair<QTime, MTMessage> > mapHearts; //已经发送的任务的上次心跳时间
    QQueue<MTMessage> lstTasks;
    // 能一次发送的消息
    for (int i = 0; i < m_TaskCnt-m_multiTaskCnt; i++)
    {
        ++nSendSeq;
        MTMessage msSend = {mtTask, QUuid::createUuid().toString(), uRID, nSendSeq, "", 0, 1, 0, infString, QString("send work from %1 for info:%2")
                            .arg(ZMDUtils::lastAddr(uUID)).arg(nSendSeq).toLocal8Bit()}; //注意这里的srcAddr传的是sckExchanger的sRID
        mapTasks.insert(nSendSeq, msSend);
        lstTasks.enqueue(msSend);
    }
    // 需要多次发送的消息：首先发送一条Head信息并接收返回信息来确定Worker的地址，然后对方通过Query来查询之后的信息
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

    qDebug() << QStringLiteral("开始寻找Worker处理任务") << QTime::currentTime().toString();
    // 信息交互
    QQueue<QString> sWorkersList;
    int nResultCnt = 0;
    while (true)
    {
        // 重复广播消息
        if (QTime::currentTime() >= lastBroadCast)
        {
            doBroadCast(cWorkerPortUDPBroadCasts);
            lastBroadCast = QTime::currentTime().addMSecs(c_BroadCastLoop);
        }

        zmq_poll (items, 1, c_RouterProxyLoop);
        if (items [0].revents & ZMQ_POLLIN) // 对工人身份排队以用于负载均衡，并转发后端到前端的消息
        {
            // 信封前缀格式
            QString srcAddr = ZMDUtils::resv(&pBackSocket);

            // 第三帧是ready指令或空消息
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
//                qDebug() << QStringLiteral("尝试删除超时Worker") << srcAddr;
                if (sWorkersList.removeOne(srcAddr)) //发送确认删除的信号
                {
                    // 封装信封
                    ZMDUtils::sendmore(&pBackSocket, srcAddr);
                    ZMDUtils::send(&pBackSocket, c_Delete);
                }
                else
                {
                    ZMDUtils::sendmore(&pHeartSocket, sHeartAddr);
                    ZMDUtils::send(&pHeartSocket, c_HeartBit);
                }

            }
            else //转发后端到前端的消息
            {
                MTMessage mtMsg = ZMDUtils::resvMsg(&pBackSocket);
    //            qDebug() << QStringLiteral("返回信息") << mtMsg.toString();
                QString srcAddr = mtMsg.srcAddr;
                switch (mtMsg.mtType) {
                case mtResult:
                    nResultCnt++;
                    if (mapTasks.keys().contains(mtMsg.ackNo))
                    {
                        qDebug() << QStringLiteral("接收结果") << ZMDUtils::lastAddr(uRID) << ZMDUtils::lastAddr(srcAddr) << mtMsg.info ;
                        mapTasks.remove(mtMsg.ackNo);
                        mapHearts.remove(mtMsg.srcAddr);
                    }
                    break;
                case mtQuery:
                    {
                        ++nSendSeq;
                        qDebug() << QStringLiteral("接收询问") << ZMDUtils::lastAddr(uRID) << ZMDUtils::lastAddr(srcAddr) << mtMsg.info << mtMsg.sequence;
                        int nSleep = qrand() % 100;
                        Sleep(nSleep); //!!!! 查询新的数据
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
            // 封装信封
            ZMDUtils::sendmore(&pBackSocket, dstAddr);
            ZMDUtils::sendmore(&pBackSocket, "");
            ZMDUtils::sendMsg(&pBackSocket, msSend);
            mapHearts.insert(dstAddr, qMakePair(QTime::currentTime().addMSecs(c_TimeHeartBit), msSend));
            qDebug() << QStringLiteral("发送一个新任务给") << msSend.toString() << QTime::currentTime().toString();;
        }
        if (mapTasks.isEmpty())
        {
            qDebug() << QStringLiteral("完成运算");
            break;
        }
        reOrgSendQQueue(lstTasks, mapHearts);
    }

    qDebug() << QStringLiteral("结束运算");
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
                qDebug() << QStringLiteral("超时任务") << mapKeys.at(i) << QTime::currentTime();
                lstTasks.enqueue(mapTasks.value(mapKeys.at(i)).second);
                QPair<QTime, MTMessage> aHeart = mapTasks.value(mapKeys.at(i));
                aHeart.first = QTime::currentTime().addMSecs(c_TimeHeartBit);
                mapTasks.insert(mapKeys.at(i), aHeart);
            }

        }
    }
}
