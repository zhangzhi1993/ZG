#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQueue>
#include "ZMDUtils.h"

class QPushButton;
class QUdpSocket;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setTaskCnt(int taskCnt, int multiTaskCnt);

public slots:
    void read_msg();
    void createClient();

private:
    void reSendInfo(zmq::socket_t *socket, const QMap<int, MTMessage> &lstTasks, int revNo);

private:
    Ui::MainWindow *ui;
    QUdpSocket *us;
    QPushButton *aButton;
    QQueue<QString> proxAddrs;
    int m_TaskCnt;
    int m_multiTaskCnt;
};

#endif // MAINWINDOW_H
