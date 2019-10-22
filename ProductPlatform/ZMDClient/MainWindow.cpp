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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    aButton(NULL),
    m_TaskCnt(10),
    m_multiTaskCnt(2)
{
    ui->setupUi(this);
    us = new QUdpSocket;
    bool bConn = false;
    int nPortUDPBroadCast;
    foreach (nPortUDPBroadCast, cPortUDPBroadCasts) {
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
    connect(us, SIGNAL(readyRead()), this, SLOT(read_msg()));

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

void MainWindow::createClient()
{
    QString uUID = QUuid::createUuid().toString();
    QString uRID = ZMDUtils::refreshAddr(uUID);
    std::string sUID = uUID.toStdString();
    std::string sRID = uRID.toStdString();
    qDebug() << "createClient" << uUID << uRID;

    zmq::context_t context(1);

    // �������ߣ�����client����������������server��ÿ�뷢��һ������ͬʱ�ռ��ʹ�ӡӦ����Ϣ
    zmq::socket_t sckProducer(context, ZMQ_DEALER);
//    ZMDCommon::setHighWaterLevel(&sckProducer, 1);
    sckProducer.setsockopt(ZMQ_IDENTITY, sUID.data(), sUID.size());

    // ���ں�Worker������Ϣ
    zmq::socket_t sckExchanger(context, ZMQ_DEALER);
//    ZMDCommon::setHighWaterLevel(&sckExchanger, 1);
    sckExchanger.setsockopt(ZMQ_IDENTITY, sRID.data(), sRID.size());


    while(true)
    {
        if (proxAddrs.empty())
        {
            qDebug() << QStringLiteral("û�п��õĴ�������");
            return;
        }
        QString sIP = proxAddrs.dequeue();
        bool bConn1 = ZMDUtils::tryConnect(&sckProducer, QString("tcp://%1:%2").arg(sIP).arg(cPortClient_Proxy));
        bool bConn2 = ZMDUtils::tryConnect(&sckExchanger, QString("tcp://%1:%2").arg(sIP).arg(cPortClient_Server));
        if (bConn1 && bConn2)
        {
            qDebug() << QStringLiteral("�ɹ����ӵ����� %1").arg(sIP);
            break;
        }
        else
        {
            sckProducer.disconnect(QString("tcp://%1:%2").arg(sIP).arg(cPortClient_Proxy).toStdString());
            sckExchanger.disconnect(QString("tcp://%1:%2").arg(sIP).arg(cPortClient_Server).toStdString());
        }
    }

    ZMDUtils::printHighWaterLevel(&sckProducer);
    ZMDUtils::printHighWaterLevel(&sckExchanger);

    zmq_pollitem_t items [] = {
        { sckProducer,  0, ZMQ_POLLIN, 0 },
        { sckExchanger, 0, ZMQ_POLLIN, 0 }
    };

    int nSendSeq = 0;

    // ��һ�η��͵���Ϣ
    for (int i = 0; i < m_TaskCnt-m_multiTaskCnt; i++)
    {
        // ���������������
        ZMDUtils::sendmore(&sckProducer, "");
        MTMessage msSend = {mtTask, uRID, "", 1, 0, infString, QString("send work from %1 for info:%2")
                            .arg(ZMDUtils::lastAddr(uUID)).arg(++nSendSeq).toLocal8Bit()}; //ע�������srcAddr������sckExchanger��sRID
        bool bRes = ZMDUtils::sendMsg(&sckProducer, msSend);
        qDebug() << QStringLiteral("��������") << msSend.toString() << bRes;
    }

    // ��Ҫ��η��͵���Ϣ�����ȷ���һ��Head��Ϣ�����շ�����Ϣ��ȷ��Worker�ĵ�ַ��Ȼ��Է�ͨ��Query����ѯ֮�����Ϣ
    for (int i = 0; i < m_multiTaskCnt; i++)
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
        if (nResultCnt == m_TaskCnt)
        {
            break;
        }
    }

    qDebug() << QStringLiteral("�������");
    sckProducer.close();
    sckExchanger.close();
    context.close();
}
