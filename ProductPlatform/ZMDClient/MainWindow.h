#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTextEdit>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void read_msg();

private:
    Ui::MainWindow *ui;
    QUdpSocket *us;
    QTextEdit *plainTextEdit;
};

#endif // MAINWINDOW_H
