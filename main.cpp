#include "mainwindow.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用信息
    QApplication::setApplicationName("个人任务管理系统");
    QApplication::setApplicationVersion("1.0");

    qDebug() << "应用程序启动...";
    qDebug() << "Qt版本：" << qVersion();

    MainWindow w;
    qDebug() << "主窗口创建完成";

    w.show();
    qDebug() << "主窗口显示完成，进入事件循环";

    return a.exec();
}
