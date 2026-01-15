#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

bool checkDatabaseDrivers() {
    qDebug() << "检查可用数据库驱动：";
    QStringList drivers = QSqlDatabase::drivers();
    qDebug() << "可用驱动：" << drivers;

    bool hasSQLite = drivers.contains("QSQLITE");
    if (!hasSQLite) {
        qCritical() << "未找到SQLite驱动！";
    } else {
        qDebug() << "找到SQLite驱动";
    }

    return hasSQLite;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 检查数据库驱动
    if (!checkDatabaseDrivers()) {
        QMessageBox::critical(nullptr, "错误",
                              "未找到SQLite数据库驱动！\n"
                              "请确保已安装Qt SQL模块。\n"
                              "程序无法正常运行。");
        return 1;
    }

    // 设置应用信息
    QApplication::setApplicationName("个人任务管理系统");
    QApplication::setApplicationVersion("1.0");

    qDebug() << "=== 应用程序启动 ===";

    try {
        MainWindow w;
        qDebug() << "主窗口创建完成";

        w.show();
        qDebug() << "主窗口显示完成，进入事件循环";

        return a.exec();
    } catch (const std::exception& e) {
        qCritical() << "程序异常：" << e.what();
        QMessageBox::critical(nullptr, "程序崩溃",
                              QString("程序发生异常：\n%1").arg(e.what()));
        return -1;
    } catch (...) {
        qCritical() << "未知程序异常";
        QMessageBox::critical(nullptr, "程序崩溃", "程序发生未知异常");
        return -1;
    }
}
