#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQueue>

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

public slots:
    void read_msg();
    void createClient();

private:
    Ui::MainWindow *ui;
    QUdpSocket *us;
    QPushButton *aButton;
    QQueue<QString> proxAddrs;
};

#endif // MAINWINDOW_H
