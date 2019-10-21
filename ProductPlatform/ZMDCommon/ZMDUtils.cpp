#include "ZMDUtils.h"
#include <QFile>
#include <QDataStream>
#include <QUuid>
#include <QDebug>
#include "MTMessage.h"

ZMDUtils::ZMDUtils()
{

}

QString ZMDUtils::resv(zmq::socket_t *socket)
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

MTMessage ZMDUtils::resvMsg(zmq::socket_t *socket)
{
    MSGTYPE mtType = (MSGTYPE)ZMDUtils::resv(socket).toInt();
    QString sSrcAddr = ZMDUtils::resv(socket);
    QString sdstAddr = ZMDUtils::resv(socket);
    int nTotal = ZMDUtils::resv(socket).toInt();
    int nSequence = ZMDUtils::resv(socket).toInt();
    INFOTYPE infType = (INFOTYPE)ZMDUtils::resv(socket).toInt();
    QByteArray sMsg = ZMDUtils::resvByteArr(socket);
    MTMessage mtMsg = {mtType, sSrcAddr, sdstAddr, nTotal, nSequence, infType, sMsg};
    return mtMsg;
}

bool ZMDUtils::sendMsg(zmq::socket_t *socket, MTMessage mtMsg)
{
    bool bRes = ZMDUtils::sendmore(socket, QString::number(mtMsg.mtType));
    bRes = bRes && ZMDUtils::sendmore(socket, mtMsg.srcAddr);
    bRes = bRes && ZMDUtils::sendmore(socket, mtMsg.dstAddr);
    bRes = bRes && ZMDUtils::sendmore(socket, QString::number(mtMsg.total));
    bRes = bRes && ZMDUtils::sendmore(socket, QString::number(mtMsg.sequence));
    bRes = bRes && ZMDUtils::sendmore(socket, QString::number(mtMsg.infType));
    bRes = bRes && ZMDUtils::sendByteArr(socket, mtMsg.info);
    return bRes;
}

bool ZMDUtils::send(zmq::socket_t *socket, QString msg)
{
    std::string stdStr = msg.toStdString();
    zmq::message_t request(stdStr.size());
    memcpy(request.data(), stdStr.data(), stdStr.size());
    return socket->send(request);
}

bool ZMDUtils::sendmore(zmq::socket_t *socket, QString msg)
{
    std::string stdStr = msg.toStdString();
    zmq::message_t request(stdStr.size());
    memcpy(request.data(), stdStr.data(), stdStr.size());
    return socket->send(request, ZMQ_SNDMORE);
}

QString ZMDUtils::lastAddr(QString addr)
{
    return "{" + addr.right(5);
}

QString ZMDUtils::refreshAddr(QString addr)
{
    int nLen = addr.length();
    int nRef = 5;
    QString uUID = QUuid::createUuid().toString();
    return addr.left(nLen-nRef) + uUID.right(nRef);
}

void ZMDUtils::setHighWaterLevel(zmq::socket_t *socket, int highWaterLevel)
{
    size_t nSize = sizeof(int);
    socket->setsockopt(ZMQ_RCVHWM, &highWaterLevel, nSize);
    socket->setsockopt(ZMQ_SNDHWM, &highWaterLevel, nSize);
}

QByteArray ZMDUtils::resvByteArr(zmq::socket_t *socket)
{
    zmq::message_t request;
    socket->recv(&request);

    char *recvData = (char *)malloc(request.size());
    memcpy(recvData, request.data(), request.size());
    QByteArray sMsg(recvData, request.size());
    free(recvData);
    return sMsg;
}

bool ZMDUtils::sendByteArr(zmq::socket_t *socket, QByteArray msg)
{
    zmq::message_t request(msg.size());
    memcpy(request.data(), msg.data(), msg.size());
    return socket->send(request);
}

void ZMDUtils::printHighWaterLevel(zmq::socket_t *socket)
{
    int highRcvWaterLevel = 0;
    int highRndWaterLevel = 0;
    size_t nSize = sizeof(int);
    socket->getsockopt(ZMQ_RCVHWM, &highRcvWaterLevel, &nSize);
    socket->getsockopt(ZMQ_SNDHWM, &highRndWaterLevel, &nSize);
    qDebug() << "ZMQ_RCVHWM" << highRcvWaterLevel << "ZMQ_SNDHWM" << highRndWaterLevel;
}

bool ZMDUtils::readFileString(QString path, QByteArray &data)
{
    if(path.isEmpty() == false)
    {
        QFile file(path);
        bool ret = file.open(QIODevice::ReadOnly);
        if(ret == true)
        {
            //将文件内容读出
            data = file.readAll();
            file.close();
            return true;
        }
    }
    return false;
}

bool ZMDUtils::writeFileString(QString path, const QByteArray &data)
{
    if(path.isEmpty() == false)
    {
        QFile file(path);
        bool ret = file.open(QIODevice::WriteOnly);
        if(ret == true)
        {
            //将文件内容读出
            file.write(data, data.size());
            file.close();
            return true;
        }
    }
    return false;
}

