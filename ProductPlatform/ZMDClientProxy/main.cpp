#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    if (argc >= 3)
    {
        w.setTaskCnt(QString(argv[1]).toInt(), QString(argv[2]).toInt());
    }
    w.show();
    return a.exec();
}
