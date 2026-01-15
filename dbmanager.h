#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>
#include <QMutex>
#include "task.h"

class DBManager : public QObject
{
    Q_OBJECT
public:
    static DBManager* instance();
    ~DBManager() override;

    bool initDatabase();
    bool addTask(const Task& task);
    bool updateTask(const Task& task);
    bool deleteTask(int taskId);
    QList<Task> getAllTasks() const;
    Task getTaskById(int taskId) const;
    bool isDatabaseOpen() const { return m_db.isOpen(); }

    // 设置数据库路径
    void setDatabasePath(const QString& path);
    QString getDatabasePath() const { return m_db.databaseName(); }

private:
    explicit DBManager(QObject *parent = nullptr);

    static DBManager* m_instance;
    static QMutex m_instanceMutex;
    QSqlDatabase m_db;
    mutable QMutex m_mutex;
};

#endif // DBMANAGER_H
