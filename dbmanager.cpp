#include "dbmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDir>

DBManager *DBManager::m_instance = nullptr;

DBManager::DBManager(QObject *parent)
    : QObject(parent)
{
    // 初始化SQLite连接（修复拼写：QSqIDatabase → QSqlDatabase）
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    // 数据库存放在用户主目录（避免权限问题）
    m_db.setDatabaseName(QDir::homePath() + "/" + DB_NAME);
}

DBManager::~DBManager()
{
    QMutexLocker locker(&m_mutex);
    if (m_db.isOpen())
        m_db.close();
}

DBManager *DBManager::getInstance(QObject *parent)
{
    if (!m_instance)
        m_instance = new DBManager(parent);
    return m_instance;
}

bool DBManager::initDB()
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.open()) {
        emit dbError("数据库打开失败：" + m_db.lastError().text());
        return false;
    }

    // 创建任务表（id自增主键）
    QSqlQuery query;
    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            deadline TEXT NOT NULL,
            priority INTEGER NOT NULL DEFAULT 1,
            is_completed INTEGER NOT NULL DEFAULT 0,
            description TEXT DEFAULT ""
        )
    )";

    if (!query.exec(createTableSql)) {
        emit dbError("创建任务表失败：" + query.lastError().text());
        return false;
    }
    return true;
}

bool DBManager::addTask(const Task &task)
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;

    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO tasks (title, deadline, priority, is_completed, description)
        VALUES (:title, :deadline, :priority, :is_completed, :description)
    )");
    query.bindValue(":title", task.title);
    query.bindValue(":deadline", task.deadline.toString("yyyy-MM-dd HH:mm"));
    query.bindValue(":priority", task.priority);
    query.bindValue(":is_completed", task.isCompleted ? 1 : 0);
    query.bindValue(":description", task.description);

    if (!query.exec()) {
        emit dbError("添加任务失败：" + query.lastError().text());
        return false;
    }
    return true;
}

bool DBManager::updateTask(const Task &task)
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;

    QSqlQuery query;
    query.prepare(R"(
        UPDATE tasks
        SET title = :title, deadline = :deadline, priority = :priority,
            is_completed = :is_completed, description = :description
        WHERE id = :id
    )");
    query.bindValue(":title", task.title);
    query.bindValue(":deadline", task.deadline.toString("yyyy-MM-dd HH:mm"));
    query.bindValue(":priority", task.priority);
    query.bindValue(":is_completed", task.isCompleted ? 1 : 0);
    query.bindValue(":description", task.description);
    query.bindValue(":id", task.id);

    if (!query.exec()) {
        emit dbError("更新任务失败：" + query.lastError().text());
        return false;
    }
    return true;
}

bool DBManager::deleteTask(int taskId)
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;

    QSqlQuery query;
    query.prepare("DELETE FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);

    if (!query.exec()) {
        emit dbError("删除任务失败：" + query.lastError().text());
        return false;
    }
    return true;
}

QList<Task> DBManager::getAllTasks()
{
    QList<Task> tasks;
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return tasks;

    QSqlQuery query("SELECT * FROM tasks ORDER BY deadline ASC");
    while (query.next()) {
        Task task;
        task.id = query.value("id").toInt();
        task.title = query.value("title").toString();
        task.deadline = QDateTime::fromString(query.value("deadline").toString(), "yyyy-MM-dd HH:mm");
        task.priority = query.value("priority").toInt();
        task.isCompleted = query.value("is_completed").toInt() == 1;
        task.description = query.value("description").toString();
        tasks.append(task);
    }
    return tasks;
}

int DBManager::getNextTaskId()
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return 1;

    QSqlQuery query("SELECT MAX(id) FROM tasks");
    if (query.exec() && query.next()) {
        int maxId = query.value(0).toInt();
        return maxId + 1;
    }
    return 1; // 无数据时从1开始
}
