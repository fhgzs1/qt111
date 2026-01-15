#include "taskmodel.h"
#include <QColor>
#include <QDebug>
#include <QMutexLocker>

TaskModel::TaskModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    qDebug() << "TaskModel构造函数开始";
    refreshTasks();
    qDebug() << "TaskModel构造函数结束，任务数：" << m_cachedTasks.size();
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_cachedTasks.size();
}

int TaskModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ColumnCount;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_cachedTasks.size())
        return QVariant();

    const Task &task = m_cachedTasks[index.row()];

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (index.column()) {
        case ColumnTitle: return task.title;
        case ColumnDeadline: return task.deadline.toString("yyyy-MM-dd HH:mm");
        case ColumnPriority:
            return (task.priority == 0 ? "低" : (task.priority == 1 ? "中" : "高"));
        case ColumnCompleted: return task.isCompleted ? "已完成" : "未完成";
        default: return QVariant();
        }
    case Qt::ForegroundRole:
        if (task.isCompleted) return QColor(Qt::gray);
        if (task.priority == 2) return QColor(Qt::red);
        return QColor(Qt::black);
    case Qt::CheckStateRole:
        if (index.column() == ColumnCompleted)
            return task.isCompleted ? Qt::Checked : Qt::Unchecked;
    default:
        return QVariant();
    }
}

QVariant TaskModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ColumnTitle: return "任务标题";
        case ColumnDeadline: return "截止时间";
        case ColumnPriority: return "优先级";
        case ColumnCompleted: return "完成状态";
        default: return QVariant();
        }
    }
    return QVariant();
}

bool TaskModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_cachedTasks.size())
        return false;

    Task &task = m_cachedTasks[index.row()];
    bool changed = false;

    if (role == Qt::CheckStateRole && index.column() == ColumnCompleted) {
        task.isCompleted = (value.toInt() == Qt::Checked);
        changed = true;
    }

    if (changed) {
        DBManager::instance()->updateTask(task);
        emit dataChanged(index, index, {role});
        emit taskDataChanged();
        return true;
    }
    return false;
}

Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    if (index.isValid()) {
        flags |= Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        if (index.column() == ColumnCompleted) {
            flags |= Qt::ItemIsUserCheckable;
        }
    }
    return flags;
}

void TaskModel::addTask(const Task &task)
{
    qDebug() << "添加任务：" << task.title;

    if (DBManager::instance()->addTask(task)) {
        refreshTasks();
        emit taskDataChanged();
    }
}

void TaskModel::updateTask(const Task &task)
{
    qDebug() << "更新任务：" << task.title;

    if (DBManager::instance()->updateTask(task)) {
        refreshTasks();
        emit taskDataChanged();
    }
}

void TaskModel::removeTask(int taskId)
{
    qDebug() << "删除任务ID：" << taskId;

    if (DBManager::instance()->deleteTask(taskId)) {
        refreshTasks();
        emit taskDataChanged();
    }
}

void TaskModel::toggleTaskCompleted(int taskId)
{
    Task task = getTaskById(taskId);
    if (task.id != -1) {
        task.isCompleted = !task.isCompleted;
        updateTask(task);
    }
}

void TaskModel::refreshTasks()
{
    qDebug() << "刷新任务列表...";

    beginResetModel();
    m_cachedTasks = DBManager::instance()->getAllTasks();
    endResetModel();

    qDebug() << "刷新完成，任务数：" << m_cachedTasks.size();
}

QList<Task> TaskModel::getAllTasks() const
{
    return m_cachedTasks;
}

Task TaskModel::getTaskById(int taskId) const
{
    for (const auto &task : m_cachedTasks) {
        if (task.id == taskId) {
            return task;
        }
    }
    return Task();
}
