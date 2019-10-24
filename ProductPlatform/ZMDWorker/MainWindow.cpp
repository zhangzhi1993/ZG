#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QByteArray>
#include <QHostAddress>
#include <QNetworkAddressEntry>
#include <QDebug>
#include <QUuid>
#include <QString>
#include <QDateTime>
#include <QVBoxLayout>
#include <QUdpSocket>
#include <QPushButton>
#include <iostream>
#include "czmq.h"
#include "ZMDUtils.h"
#include "MTMessage.h"
#include "zhelpers.h"
#include "ZMDConst.h"

const int c_Retry = 10;
const int c_Sleep = 1000;
const int c_PipeLine = 5;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    aButton(NULL),
    bInWorking(false)
{
    ui->setupUi(this);
    us = new QUdpSocket;
    bool bConn = false;
    int nPortUDPBroadCast;
    foreach (nPortUDPBroadCast, cWorkerPortUDPBroadCasts) {
        if (us->bind(nPortUDPBroadCast, QUdpSocket::ShareAddress))
        {
            bConn = true;
            break;
        }
    }
    if (!bConn)
    {
        qDebug() << QStringLiteral("连接广播端口失败");
        return;
    }
    else
    {
        qDebug() << QStringLiteral("成功连接广播端口 %1").arg(nPortUDPBroadCast);
    }
    connect(us, SIGNAL(readyRead()), this, SLOT(createWorker()));

    QWidget *centerWindow = new QWidget;
    this->setCentralWidget(centerWindow);

    QVBoxLayout *aLayout = new QVBoxLayout;
    aButton = new QPushButton();
    aButton->setText(QStringLiteral("一个新任务"));
    aLayout->addWidget(aButton);
    centerWindow->setLayout(aLayout);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete us;
    delete aButton;
}

void MainWindow::read_msg()
{
    QByteArray ba;
    while(us->hasPendingDatagrams())
    {
        ba.resize(us->pendingDatagramSize());
        us->readDatagram(ba.data(), ba.size());

        QString sProxAddr = QString(ba).split(c_Separator).first();
        proxAddrs.removeOne(sProxAddr);
        proxAddrs.enqueue(sProxAddr);
    }
}

void MainWindow::createWorker()
{
//    while (1)
    if (!bInWorking)
    {
        bInWorking = true;
        zmq::context_t context(1);
        if (us->hasPendingDatagrams())
        {
            QByteArray ba;
            ba.resize(us->pendingDatagramSize());
            us->readDatagram(ba.data(), ba.size());

            QString sProxAddr = QString(ba).split(c_Separator).first();
            proxAddrs.removeOne(sProxAddr);
            proxAddrs.enqueue(sProxAddr);
        }
        else
        {
//            continue;
            bInWorking = false;
            return;
        }

        QString uUID = QUuid::createUuid().toString();
        std::string sUID = uUID.toStdString();
        qDebug() << "createWorker" << uUID;

        // 任务处理器
        zmq::socket_t sckConsumer (context, ZMQ_DEALER);
        sckConsumer.setsockopt(ZMQ_IDENTITY, sUID.data(), sUID.size());

        while(true)
        {
            if (proxAddrs.empty())
            {
                qDebug() << QStringLiteral("没有可用的代理！！！");
                return;
            }
            QString sIP = proxAddrs.dequeue();
            bool bConn = ZMDUtils::tryConnect(&sckConsumer, QString("tcp://%1:%2").arg(sIP).arg(cPortServer_Proxy));
            if (bConn)
            {
                qDebug() << QStringLiteral("成功连接到代理 %1").arg(sIP);
                break;
            }
            else
            {
                sckConsumer.disconnect(QString("tcp://%1:%2").arg(sIP).arg(cPortServer_Proxy).toStdString());
            }
        }


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
        context.close();
    }

    bInWorking = false;


}
