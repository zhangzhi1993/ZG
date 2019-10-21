#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QByteArray>
#include <QHostAddress>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    plainTextEdit(NULL)
{
    ui->setupUi(this);
    us = new QUdpSocket;
    us->bind(3333, QUdpSocket::ShareAddress);
    connect(us, SIGNAL(readyRead()), this, SLOT(read_msg()));
    plainTextEdit = new QTextEdit();
    this->setCentralWidget(plainTextEdit);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete us;
}

void MainWindow::read_msg()
{
    QByteArray ba;
    while(us->hasPendingDatagrams())
    {
        ba.resize(us->pendingDatagramSize());
        us->readDatagram(ba.data(), ba.size());
        plainTextEdit->append(QString(ba));
    }
}
