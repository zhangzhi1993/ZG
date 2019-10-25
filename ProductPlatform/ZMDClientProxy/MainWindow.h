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
    void createClient();
private slots:
    void doBroadCast(QList<int> sPortUDPBroadCasts);

private:
    void reOrgSendQQueue(QQueue<MTMessage> &lstTasks, QMap<QString, QPair<QTime, MTMessage> > &mapTasks);

private:
    Ui::MainWindow *ui;
    QUdpSocket *us;
    QPushButton *aButton;
    int m_TaskCnt;
    int m_multiTaskCnt;
};

#endif // MAINWINDOW_H
