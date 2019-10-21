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
    /* �����͹㲥�������ڵڶ������� */
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
                us->writeDatagram(ba.toStdString().data(), broadcastAddress, 3333);  // UDP ��������
            }
        }
     }
}

void MainWindow::createRouterProxy()
{
    zmq::context_t context(1);

    zmq::socket_t pFrontSocket1 (context, ZMQ_ROUTER);
//    ZMDCommon::setHighWaterLevel(&pFrontSocket1, 1);
    pFrontSocket1.bind(QString("tcp://*:%1").arg(cPortClient_Server).toStdString()); //����ָ����Client������Ϣ��ָ����Worker

    zmq::socket_t pFrontSocket2 (context, ZMQ_ROUTER);
//    ZMDCommon::setHighWaterLevel(&pFrontSocket2, 1);
    pFrontSocket2.bind(QString("tcp://*:%1").arg(cPortClient_Proxy).toStdString()); //ǰ�˰�;ֻ���ڷ������񣬼���ʱClient��֪���ú��ĸ�Worker��ϵ

    zmq::socket_t pBackSocket (context, ZMQ_ROUTER);
//    ZMDCommon::setHighWaterLevel(&pBackSocket, 1);
    pBackSocket.bind(QString("tcp://*:%1").arg(cPortServer_Proxy).toStdString());//��˰�;

    ZMDUtils::printHighWaterLevel(&pFrontSocket1);
    ZMDUtils::printHighWaterLevel(&pFrontSocket2);
    ZMDUtils::printHighWaterLevel(&pBackSocket);

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
            QString srcAddr = ZMDUtils::resv(&pBackSocket);

            // ����֡��readyָ������Ϣ
            QString sReady = ZMDUtils::resv(&pBackSocket);
            if (sReady != c_Ready) //ת����˵�ǰ�˵���Ϣ
            {
                MTMessage msResv = ZMDUtils::resvMsg(&pBackSocket);
                QString dstAddr = msResv.dstAddr;

//                qDebug() << "pBackSocket" << msResv.toString();
                // ��װ�ŷ�
                ZMDUtils::sendmore(&pFrontSocket1, dstAddr);
                ZMDUtils::sendmore(&pFrontSocket1, "");
                ZMDUtils::sendMsg(&pFrontSocket1, msResv);
            }
            else
            {
                sWorkersList.enqueue(srcAddr);
            }
        }
        if (items [1].revents & ZMQ_POLLIN) // ת��ǰ�˵���˵���Ϣ
        {
            // �ŷ�ǰ׺��ʽ
            QString srcAddr = ZMDUtils::resv(&pFrontSocket1);
            ZMDUtils::resv(&pFrontSocket1); //�ڶ�֡�ǿ�

             // ָ����Client��ָ����Worker��ϵ��Ŀ���ַ��Ϊ��
            MTMessage msResv = ZMDUtils::resvMsg(&pFrontSocket1);

            QString dstAddr = msResv.dstAddr;
            // ��װ�ŷ�
            ZMDUtils::sendmore(&pBackSocket, dstAddr);
            ZMDUtils::sendMsg(&pBackSocket, msResv);
        }
        if (items [2].revents & ZMQ_POLLIN && !sWorkersList.isEmpty()) // ���ڿͻ��˷�������
        {
            // �ŷ�ǰ׺��ʽ
            QString srcAddr = ZMDUtils::resv(&pFrontSocket2);
            ZMDUtils::resv(&pFrontSocket2); //�ڶ�֡�ǿ�

             // �����Ľ�����Ϣ�����������������Ϣ��Ŀ���ַΪ��
            MTMessage msResv = ZMDUtils::resvMsg(&pFrontSocket2);
            QString dstAddr = sWorkersList.dequeue();
            msResv.dstAddr = dstAddr;
//            qDebug() << "pFrontSocket" << msResv.toString();
            // ��װ�ŷ�
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
