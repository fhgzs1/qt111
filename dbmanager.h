#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include "taskmodel.h"
#include <QMutex>

class DBManager : public QObject
{
    Q_OBJECT
public:
    static DBManager* getInstance(QObject *parent = nullptr); // 单例模式
    ~DBManager() override;

    bool initDB();                          // 初始化数据库（创建表）
    bool addTask(const Task &task);         // 添加任务到数据库
    bool updateTask(const Task &task);      // 更新数据库中的任务
    bool deleteTask(int taskId);            // 从数据库删除任务
    QList<Task> getAllTasks();              // 从数据库获取所有任务
    int getNextTaskId();                    // 获取下一个自增ID

signals:
    void dbError(const QString &errorMsg);  // 数据库错误信号

private:
    explicit DBManager(QObject *parent = nullptr);
    static DBManager *m_instance;
    QSqlDatabase m_db;                      // 修复拼写：QSqIDatabase → QSqlDatabase
    const QString DB_NAME = "TaskDB.sqlite";// 数据库文件名
    mutable QMutex m_mutex;                 // 线程安全锁
};

#endif // DBMANAGER_H
