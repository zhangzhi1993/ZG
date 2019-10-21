#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QByteArray>
#include <QHostAddress>
#include <QNetworkAddressEntry>
#include <QQueue>
#include "czmq.h"
#include <QDebug>
#include <iostream>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/event_struct.h>
#include <QString>
#include "ZMDUtils.h"
#include "MTMessage.h"
#include "zhelpers.h"
#include "ZMDConst.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    abutton(NULL)
{
    ui->setupUi(this);
    us = new QUdpSocket;
    abutton = new QPushButton;
    this->setCentralWidget(abutton);
    connect(abutton, SIGNAL(clicked(bool)), this, SLOT(doBroadCast()));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete us;
}

void MainWindow::doBroadCast()
{
    /* 单播和广播的区别在第二个参数 */
    QList<QNetworkInterface> networkinterfaces = QNetworkInterface::allInterfaces();
    int i = 0;
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
                QString ba = QString("%1").arg(entry.ip().toString());
                us->writeDatagram(ba.toStdString().data(), broadcastAddress, 3333);  // UDP 发送数据
            }
        }
     }
}

void MainWindow::createRouterProxy()
{
    zmq::context_t context(1);

    zmq::socket_t pFrontSocket1 (context, ZMQ_ROUTER);
//    ZMDCommon::setHighWaterLevel(&pFrontSocket1, 1);
    pFrontSocket1.bind(QString("tcp://*:%1").arg(cPortClient_Server).toStdString()); //用于指定的Client发送消息到指定的Worker

    zmq::socket_t pFrontSocket2 (context, ZMQ_ROUTER);
//    ZMDCommon::setHighWaterLevel(&pFrontSocket2, 1);
    pFrontSocket2.bind(QString("tcp://*:%1").arg(cPortClient_Proxy).toStdString()); //前端绑定;只用于发送任务，即此时Client不知道该和哪个Worker联系

    zmq::socket_t pBackSocket (context, ZMQ_ROUTER);
//    ZMDCommon::setHighWaterLevel(&pBackSocket, 1);
    pBackSocket.bind(QString("tcp://*:%1").arg(cPortServer_Proxy).toStdString());//后端绑定;

    ZMDUtils::printHighWaterLevel(&pFrontSocket1);
    ZMDUtils::printHighWaterLevel(&pFrontSocket2);
    ZMDUtils::printHighWaterLevel(&pBackSocket);

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
            QString srcAddr = ZMDUtils::resv(&pBackSocket);

            // 第三帧是ready指令或空消息
            QString sReady = ZMDUtils::resv(&pBackSocket);
            if (sReady != c_Ready) //转发后端到前端的消息
            {
                MTMessage msResv = ZMDUtils::resvMsg(&pBackSocket);
                QString dstAddr = msResv.dstAddr;

//                qDebug() << "pBackSocket" << msResv.toString();
                // 封装信封
                ZMDUtils::sendmore(&pFrontSocket1, dstAddr);
                ZMDUtils::sendmore(&pFrontSocket1, "");
                ZMDUtils::sendMsg(&pFrontSocket1, msResv);
            }
            else
            {
                sWorkersList.enqueue(srcAddr);
            }
        }
        if (items [1].revents & ZMQ_POLLIN) // 转发前端到后端的信息
        {
            // 信封前缀格式
            QString srcAddr = ZMDUtils::resv(&pFrontSocket1);
            ZMDUtils::resv(&pFrontSocket1); //第二帧是空

             // 指定的Client和指定的Worker联系，目标地址不为空
            MTMessage msResv = ZMDUtils::resvMsg(&pFrontSocket1);

            QString dstAddr = msResv.dstAddr;
            // 封装信封
            ZMDUtils::sendmore(&pBackSocket, dstAddr);
            ZMDUtils::sendMsg(&pBackSocket, msResv);
        }
        if (items [2].revents & ZMQ_POLLIN && !sWorkersList.isEmpty()) // 用于客户端发送任务
        {
            // 信封前缀格式
            QString srcAddr = ZMDUtils::resv(&pFrontSocket2);
            ZMDUtils::resv(&pFrontSocket2); //第二帧是空

             // 真正的接收信息，由于是任务发起的信息，目标地址为空
            MTMessage msResv = ZMDUtils::resvMsg(&pFrontSocket2);
            QString dstAddr = sWorkersList.dequeue();
            msResv.dstAddr = dstAddr;
//            qDebug() << "pFrontSocket" << msResv.toString();
            // 封装信封
            ZMDUtils::sendmore(&pBackSocket, dstAddr);
            ZMDUtils::sendMsg(&pBackSocket, msResv);
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
