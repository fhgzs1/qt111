#include "dbmanager.h"
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QMutex>
#include <QFileInfo>
#include <QApplication>

// 静态成员初始化
DBManager* DBManager::m_instance = nullptr;
QMutex DBManager::m_instanceMutex;

// 单例函数实现
DBManager* DBManager::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_instanceMutex);
        if (!m_instance) {
            m_instance = new DBManager();
        }
    }
    return m_instance;
}

// 构造函数
DBManager::DBManager(QObject *parent) : QObject(parent)
{
    // 创建数据库连接
    m_db = QSqlDatabase::addDatabase("QSQLITE");

    // 设置默认路径：D:\Qt zy\zhsj\TaskManager.db
    QString defaultPath = "D:/Qt zy/zhsj/TaskManager.db";

    qDebug() << "默认数据库路径：" << defaultPath;

    // 确保目录存在
    QFileInfo fileInfo(defaultPath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        qDebug() << "创建目录：" << dir.absolutePath();
        if (!dir.mkpath(".")) {
            qCritical() << "无法创建目录，使用文档文件夹";
            defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                          + "/TaskManager.db";
        }
    }

    m_db.setDatabaseName(defaultPath);
}

// 设置数据库路径
void DBManager::setDatabasePath(const QString& path)
{
    QMutexLocker locker(&m_mutex);

    // 如果数据库已经打开，先关闭
    if (m_db.isOpen()) {
        m_db.close();
    }

    // 确保目录存在
    QFileInfo fileInfo(path);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        qDebug() << "创建目录：" << dir.absolutePath();
        if (!dir.mkpath(".")) {
            qCritical() << "无法创建目录：" << dir.absolutePath();
            return;
        }
    }

    // 设置新的数据库路径
    m_db.setDatabaseName(path);

    qDebug() << "数据库路径设置为：" << path;
}

// 析构函数
DBManager::~DBManager()
{
    if (m_db.isOpen()) {
        qDebug() << "关闭数据库连接";
        m_db.close();
    }
}

// 初始化数据库
bool DBManager::initDatabase()
{
    QMutexLocker locker(&m_mutex);

    qDebug() << "初始化SQLite数据库...";
    qDebug() << "数据库文件：" << m_db.databaseName();

    // 打开数据库
    if (!m_db.open()) {
        qCritical() << "打开SQLite数据库失败：" << m_db.lastError().text();
        return false;
    }

    qDebug() << "SQLite数据库打开成功！";

    // 创建表
    QSqlQuery query;
    QString createSql = R"(
        CREATE TABLE IF NOT EXISTS tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            deadline TEXT NOT NULL,
            priority INTEGER NOT NULL DEFAULT 0,
            isCompleted INTEGER NOT NULL DEFAULT 0,
            description TEXT
        )
    )";

    if (!query.exec(createSql)) {
        qCritical() << "创建表失败：" << query.lastError().text();
        return false;
    }

    qDebug() << "tasks表创建/检查完成";
    return true;
}

// 添加任务
bool DBManager::addTask(const Task& task)
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开，无法添加任务";
        return false;
    }

    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO tasks (title, deadline, priority, isCompleted, description)
        VALUES (:title, :deadline, :priority, :isCompleted, :description)
    )");
    query.bindValue(":title", task.title);
    query.bindValue(":deadline", task.deadline.toString("yyyy-MM-dd HH:mm"));
    query.bindValue(":priority", task.priority);
    query.bindValue(":isCompleted", task.isCompleted ? 1 : 0);
    query.bindValue(":description", task.description);

    if (!query.exec()) {
        qCritical() << "添加任务失败：" << query.lastError().text();
        return false;
    }

    qDebug() << "添加任务成功：" << task.title;
    return true;
}

// 更新任务
bool DBManager::updateTask(const Task& task)
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开，无法更新任务";
        return false;
    }

    if (task.id == -1) {
        qWarning() << "无效的任务ID";
        return false;
    }

    QSqlQuery query;
    query.prepare(R"(
        UPDATE tasks
        SET title = :title, deadline = :deadline, priority = :priority,
            isCompleted = :isCompleted, description = :description
        WHERE id = :id
    )");
    query.bindValue(":title", task.title);
    query.bindValue(":deadline", task.deadline.toString("yyyy-MM-dd HH:mm"));
    query.bindValue(":priority", task.priority);
    query.bindValue(":isCompleted", task.isCompleted ? 1 : 0);
    query.bindValue(":description", task.description);
    query.bindValue(":id", task.id);

    if (!query.exec()) {
        qCritical() << "更新任务失败：" << query.lastError().text();
        return false;
    }

    qDebug() << "更新任务成功：" << task.title << "(ID:" << task.id << ")";
    return true;
}

// 删除任务
bool DBManager::deleteTask(int taskId)
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开，无法删除任务";
        return false;
    }

    if (taskId == -1) {
        qWarning() << "无效的任务ID";
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);

    if (!query.exec()) {
        qCritical() << "删除任务失败：" << query.lastError().text();
        return false;
    }

    qDebug() << "删除任务成功，ID：" << taskId;
    return true;
}

// 获取所有任务
QList<Task> DBManager::getAllTasks() const
{
    QMutexLocker locker(&m_mutex);
    QList<Task> tasks;

    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开，无法获取任务";
        return tasks;
    }

    QSqlQuery query("SELECT * FROM tasks ORDER BY deadline ASC");

    while (query.next()) {
        Task task;
        task.id = query.value("id").toInt();
        task.title = query.value("title").toString();
        task.deadline = QDateTime::fromString(query.value("deadline").toString(), "yyyy-MM-dd HH:mm");
        task.priority = query.value("priority").toInt();
        task.isCompleted = query.value("isCompleted").toInt() == 1;
        task.description = query.value("description").toString();
        tasks.append(task);
    }

    qDebug() << "获取到" << tasks.size() << "个任务";
    return tasks;
}

// 按ID查任务
Task DBManager::getTaskById(int taskId) const
{
    QMutexLocker locker(&m_mutex);
    Task task;
    task.id = -1;

    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开，无法查询任务";
        return task;
    }

    if (taskId == -1) {
        return task;
    }

    QSqlQuery query;
    query.prepare("SELECT * FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);

    if (!query.exec()) {
        qCritical() << "查询任务失败：" << query.lastError().text();
        return task;
    }

    if (query.next()) {
        task.id = query.value("id").toInt();
        task.title = query.value("title").toString();
        task.deadline = QDateTime::fromString(query.value("deadline").toString(), "yyyy-MM-dd HH:mm");
        task.priority = query.value("priority").toInt();
        task.isCompleted = query.value("isCompleted").toInt() == 1;
        task.description = query.value("description").toString();
        qDebug() << "查询到任务：" << task.title << "(ID:" << task.id << ")";
    } else {
        qDebug() << "未找到任务ID：" << taskId;
    }

    return task;
}
