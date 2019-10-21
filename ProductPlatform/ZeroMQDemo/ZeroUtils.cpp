#include "ZeroUtils.h"
#include <QFile>
#include <QDataStream>
#include <QUuid>
#include <QDebug>

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
            //将文件内容读出
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
            //将文件内容读出
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
