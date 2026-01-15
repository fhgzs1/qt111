#include "taskmodel.h"
#include <QColor>
#include <QDebug>
#include <QMutexLocker>

// 构造函数：初始化时从数据库加载任务
TaskModel::TaskModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    refreshTasks();
}

// 获取表格行数
int TaskModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedTasks.size();
}

// 获取表格列数
int TaskModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ColumnCount;
}

// 核心：获取单元格数据
QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QMutexLocker locker(&m_cacheMutex);
    if (index.row() >= m_cachedTasks.size())
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

// 设置表头文本
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

// 修改单元格数据
bool TaskModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    QMutexLocker locker(&m_cacheMutex);
    if (index.row() >= m_cachedTasks.size())
        return false;

    Task task = m_cachedTasks[index.row()];
    bool changed = false;

    if (role == Qt::CheckStateRole && index.column() == ColumnCompleted) {
        task.isCompleted = (value.toInt() == Qt::Checked);
        changed = true;
    }
    else if (role == Qt::EditRole) {
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
        }
    }

    if (changed) {
        DBManager::instance()->updateTask(task);
        refreshTasks();
        emit dataChanged(index, index);
        emit taskDataChanged();
        return true;
    }
    return false;
}

// 设置单元格属性
Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    if (index.isValid()) {
        flags |= Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        if (index.column() == ColumnCompleted) {
            flags |= Qt::ItemIsUserCheckable;
        } else {
            flags |= Qt::ItemIsEditable;
        }
    }
    return flags;
}

// 添加任务
void TaskModel::addTask(const Task &task)
{
    DBManager::instance()->addTask(task);
    refreshTasks();
    emit taskDataChanged();
}

// 更新任务
void TaskModel::updateTask(const Task &task)
{
    DBManager::instance()->updateTask(task);
    refreshTasks();
    emit taskDataChanged();
}

// 删除任务
void TaskModel::removeTask(int taskId)
{
    DBManager::instance()->deleteTask(taskId);
    refreshTasks();
    emit taskDataChanged();
}

// 切换任务完成状态
void TaskModel::toggleTaskCompleted(int taskId)
{
    Task task = DBManager::instance()->getTaskById(taskId);
    if (task.id != -1) {
        task.isCompleted = !task.isCompleted;
        DBManager::instance()->updateTask(task);
        refreshTasks();
        emit taskDataChanged();
    }
}

// 刷新任务列表
void TaskModel::refreshTasks()
{
    beginResetModel();
    {
        QMutexLocker locker(&m_cacheMutex);
        m_cachedTasks = DBManager::instance()->getAllTasks();
    }
    endResetModel();
}

// 获取所有任务
QList<Task> TaskModel::getAllTasks() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedTasks;
}

// 按ID获取任务
Task TaskModel::getTaskById(int taskId) const
{
    QMutexLocker locker(&m_cacheMutex);
    for (const auto &task : m_cachedTasks) {
        if (task.id == taskId) {
            return task;
        }
    }
    return Task();
}
