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
const int c_DealerWorkerLoop = 1000;
const int c_ExpiredLoop = c_DealerWorkerLoop * 1;


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
        qDebug() << QStringLiteral("���ӹ㲥�˿�ʧ��");
        return;
    }
    else
    {
        qDebug() << QStringLiteral("�ɹ����ӹ㲥�˿� %1").arg(nPortUDPBroadCast);
    }
    connect(us, SIGNAL(readyRead()), this, SLOT(createWorker()));

    QWidget *centerWindow = new QWidget;
    this->setCentralWidget(centerWindow);

    QVBoxLayout *aLayout = new QVBoxLayout;
    aButton = new QPushButton();
    aButton->setText(QStringLiteral("һ��������"));
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
            bInWorking = false;
            return;
        }

        QString uUID = QUuid::createUuid().toString();
        std::string sUID = uUID.toStdString();
        qDebug() << "createWorker" << uUID;

        // ��������
        zmq::socket_t sckConsumer (context, ZMQ_DEALER);
        sckConsumer.setsockopt(ZMQ_IDENTITY, sUID.data(), sUID.size());

        QString sIP;
        while(true)
        {
            if (proxAddrs.empty())
            {
                qDebug() << QStringLiteral("û�п��õĴ�������");
                return;
            }
            sIP = proxAddrs.dequeue();
            bool bConn = ZMDUtils::tryConnect(&sckConsumer, QString("tcp://%1:%2").arg(sIP).arg(cPortServer_Proxy));
            if (bConn)
            {
                qDebug() << QStringLiteral("�ɹ����ӵ����� %1").arg(sIP);
                break;
            }
        }


//        qDebug() << "createWorker" << uUID << ++nCnt;

        // ���ߴ����Ѿ�׼������
        ZMDUtils::send(&sckConsumer, QString("%1%2%3").arg(c_Ready).arg(c_Separator).arg(sIP));

        zmq_pollitem_t items [] = {
            { sckConsumer, 0, ZMQ_POLLIN, 0 }
        };

        QTime nextRestart = QTime::currentTime().addMSecs(c_ExpiredLoop);
        MTMessage mtMsg;
        int nDecRetry = 0;
        while (true)
        {
            zmq_poll (items, 1, c_DealerWorkerLoop);
            if (QTime::currentTime() >= nextRestart)
            {
                ZMDUtils::send(&sckConsumer, QString("%1%2%3").arg(c_UnUsed).arg(c_Separator).arg(sIP));
                nextRestart = QTime::currentTime().addMSecs(c_ExpiredLoop);
                nDecRetry++;
                if (nDecRetry >= c_Retry)
                {
                    qDebug() << QStringLiteral("ɾ����ʱWorker") << uUID;
                    sckConsumer.disconnect(QString("tcp://%1:%2").arg(sIP).arg(cPortServer_Proxy).toStdString());
                    bInWorking = false;
                    return;
                }
            }

            if (items [0].revents & ZMQ_POLLIN)
            {
                QString sReady = ZMDUtils::resv(&sckConsumer);
                if (c_Delete == sReady)
                {
                    sckConsumer.disconnect(QString("tcp://%1:%2").arg(sIP).arg(cPortServer_Proxy).toStdString());
                    bInWorking = false;
                    return;
                }
                else
                {
                    // ��ȡ����
                    mtMsg = ZMDUtils::resvMsg(&sckConsumer);
                    break;
                }
            }
        }

        qDebug() << QStringLiteral("��������") << mtMsg.toString();
        QString srcAddr = mtMsg.srcAddr;

        int nSleep = qrand() % 100;
        Sleep(nSleep); // ��������

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
                MTMessage msSend = {mtQuery, uUID, 0, srcAddr, 0, mtMsg.total, nOffset++, infString,
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
            ZMDUtils::resv(&sckConsumer); // ��֡
            MTMessage msQuery = ZMDUtils::resvMsg(&sckConsumer);
            srcAddr = msQuery.srcAddr;
            nYetSum++;
            qDebug() << QStringLiteral("����ѯ��") << ZMDUtils::lastAddr(msQuery.dstAddr) << ZMDUtils::lastAddr(msQuery.srcAddr) << msQuery.info;
        }

        // ���ʹ�����
        bool bRes = ZMDUtils::sendmore(&sckConsumer, ""); // һ����֡�����ڴ����ж�Worker����Ϣ����
        MTMessage msSend = {mtResult, uUID, 0, srcAddr, mtMsg.synNo, 1, 0, infString, QString("send result to %1 from %2 for %3")
                            .arg(ZMDUtils::lastAddr(srcAddr)).arg(ZMDUtils::lastAddr(uUID)).arg(sSrcInfoSeq).toLocal8Bit()};
        bRes = bRes && ZMDUtils::sendMsg(&sckConsumer, msSend);
        qDebug() << QStringLiteral("���ͽ��") << msSend.toString() << bRes;
        sckConsumer.close();
        context.close();
    }

    bInWorking = false;


}
