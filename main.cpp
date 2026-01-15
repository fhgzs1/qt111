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

bool checkDirectoryAccess(const QString& path) {
    QFileInfo fileInfo(path);
    QDir dir = fileInfo.absoluteDir();

    // 检查目录是否存在
    if (!dir.exists()) {
        qDebug() << "目录不存在，尝试创建：" << dir.absolutePath();
        if (!dir.mkpath(".")) {
            qCritical() << "无法创建目录：" << dir.absolutePath();
            return false;
        }
    }

    // 检查是否有写入权限
    QString testFilePath = dir.absolutePath() + "/test_write.tmp";
    QFile testFile(testFilePath);
    if (testFile.open(QIODevice::WriteOnly)) {
        testFile.write("test");
        testFile.close();
        testFile.remove();
        qDebug() << "目录可写入：" << dir.absolutePath();
        return true;
    } else {
        qCritical() << "目录不可写入：" << dir.absolutePath();
        return false;
    }
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

    qDebug() << "应用程序启动...";
    qDebug() << "Qt版本：" << qVersion();

    // 设置数据库路径：D:\Qt zy\zhsj\TaskManager.db
    QString dbPath = "D:/Qt zy/zhsj/TaskManager.db";
    qDebug() << "数据库文件将保存在：" << dbPath;

    // 检查目录访问权限
    if (!checkDirectoryAccess(dbPath)) {
        qWarning() << "无法访问目标目录，使用文档文件夹";
        dbPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                 + "/TaskManager.db";
        qDebug() << "改用文档文件夹：" << dbPath;

        QMessageBox::warning(nullptr, "提示",
                             QString("无法访问 D:\\Qt zy\\zhsj 目录\n"
                                     "数据库文件将保存在：\n%1").arg(dbPath));
    }

    // 设置数据库路径
    DBManager* dbManager = DBManager::instance();

    // 如果默认路径不可用，使用 setDatabasePath 设置新路径
    if (dbPath != "D:/Qt zy/zhsj/TaskManager.db") {
        dbManager->setDatabasePath(dbPath);
    }

    qDebug() << "最终数据库路径：" << dbManager->getDatabasePath();

    MainWindow w;
    qDebug() << "主窗口创建完成";

    w.show();
    qDebug() << "主窗口显示完成，进入事件循环";

    return a.exec();
}
