#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 设置应用信息（优化显示）
    QApplication::setApplicationName("个人任务管理系统");
    QApplication::setApplicationVersion("1.0");

    MainWindow w;
    w.show();

    return a.exec();
}
