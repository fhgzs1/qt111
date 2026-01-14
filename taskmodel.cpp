#include "taskmodel.h"
#include <QColor>

TaskModel::TaskModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    QMutexLocker locker(&m_mutex);
    return m_tasks.size();
}

int TaskModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ColumnCount;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QMutexLocker locker(&m_mutex);
    if (index.row() >= m_tasks.size())
        return QVariant();

    const Task &task = m_tasks[index.row()];
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (index.column()) {
        case ColumnTitle: return task.title;
        case ColumnDeadline: return formatDeadline(task.deadline);
        case ColumnPriority: return priorityToString(task.priority);
        case ColumnCompleted: return task.isCompleted ? "已完成" : "未完成";
        default: return QVariant();
        }
    case Qt::ForegroundRole:  // 修复：TextColorRole 改为 ForegroundRole
        // 已完成任务置灰，高优先级任务标红
        if (task.isCompleted) return QColor(Qt::gray);
        if (task.priority == 2) return QColor(Qt::red);
        return QColor(Qt::black);
    case Qt::CheckStateRole:
        // 完成状态列显示复选框
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
    return QAbstractTableModel::headerData(section, orientation, role);
}

bool TaskModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    QMutexLocker locker(&m_mutex);
    if (index.row() >= m_tasks.size())
        return false;

    Task &task = m_tasks[index.row()];
    bool changed = false;

    if (role == Qt::CheckStateRole && index.column() == ColumnCompleted) {
        task.isCompleted = (value == Qt::Checked);
        changed = true;
    } else if (role == Qt::EditRole) {
        switch (index.column()) {
        case ColumnTitle:
            task.title = value.toString();
            changed = true;
            break;
        case ColumnDeadline:
            task.deadline = QDateTime::fromString(value.toString(), "yyyy-MM-dd HH:mm");
            changed = true;
            break;
        case ColumnPriority:
            task.priority = value.toInt();
            changed = true;
            break;
        default:
            break;
        }
    }

    if (changed) {
        emit dataChanged(index, index);
        emit taskDataChanged();
        return true;
    }
    return false;
}

Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    // 完成状态列可勾选，其他列可编辑
    if (index.column() == ColumnCompleted)
        flags |= Qt::ItemIsUserCheckable;
    else
        flags |= Qt::ItemIsEditable;
    return flags;
}

void TaskModel::addTask(const Task &task)
{
    QMutexLocker locker(&m_mutex);
    beginInsertRows(QModelIndex(), m_tasks.size(), m_tasks.size());
    m_tasks.append(task);
    endInsertRows();
    emit taskDataChanged();
}

void TaskModel::updateTask(const Task &task)
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].id == task.id) {
            m_tasks[i] = task;
            QModelIndex index = createIndex(i, 0);
            emit dataChanged(index, createIndex(i, ColumnCount - 1));
            emit taskDataChanged();
            break;
        }
    }
}

void TaskModel::removeTask(int taskId)
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].id == taskId) {
            beginRemoveRows(QModelIndex(), i, i);
            m_tasks.removeAt(i);
            endRemoveRows();
            emit taskDataChanged();
            break;
        }
    }
}

void TaskModel::toggleTaskCompleted(int taskId)
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].id == taskId) {
            m_tasks[i].isCompleted = !m_tasks[i].isCompleted;
            QModelIndex index = createIndex(i, ColumnCompleted);
            emit dataChanged(index, index);
            emit taskDataChanged();
            break;
        }
    }
}

void TaskModel::clearTasks()
{
    QMutexLocker locker(&m_mutex);
    if (!m_tasks.isEmpty()) {
        beginResetModel();
        m_tasks.clear();
        endResetModel();
        emit taskDataChanged();
    }
}

QList<Task> TaskModel::getAllTasks() const
{
    QMutexLocker locker(&m_mutex);
    return m_tasks;
}

Task TaskModel::getTaskById(int taskId) const
{
    QMutexLocker locker(&m_mutex);
    for (const auto &task : m_tasks) {
        if (task.id == taskId)
            return task;
    }
    return Task{}; // 返回空任务
}

QString TaskModel::priorityToString(int priority) const
{
    switch (priority) {
    case 0: return "低";
    case 1: return "中";
    case 2: return "高";
    default: return "未知";
    }
}

QString TaskModel::formatDeadline(const QDateTime &deadline) const
{
    return deadline.toString("yyyy-MM-dd HH:mm");
}
