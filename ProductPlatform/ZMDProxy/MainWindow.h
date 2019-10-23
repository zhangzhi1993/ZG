#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QPushButton>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void createRouterProxy();
private slots:
    void doBroadCast(QList<int> sPortUDPBroadCasts);

private:
    Ui::MainWindow *ui;
    QUdpSocket *us;
    QPushButton *abutton;
};

#endif // MAINWINDOW_H
