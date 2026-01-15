#ifndef TASKMODEL_H
#define TASKMODEL_H

#include <QAbstractTableModel>
#include "dbmanager.h"
#include "task.h"  // 包含Task结构体

class TaskModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    // 表格列定义
    enum TaskColumn {
        ColumnTitle = 0,        // 任务标题
        ColumnDeadline,         // 截止时间
        ColumnPriority,         // 优先级
        ColumnCompleted,        // 完成状态
        ColumnCount             // 列数
    };

    explicit TaskModel(QObject *parent = nullptr);

    // 重写Model核心方法
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // 任务管理接口（调用数据库）
    void addTask(const Task &task);
    void updateTask(const Task &task);
    void removeTask(int taskId);
    void toggleTaskCompleted(int taskId);
    void refreshTasks();
    QList<Task> getAllTasks() const;
    Task getTaskById(int taskId) const;

signals:
    void taskDataChanged();

private:
    // 添加一个成员变量缓存任务列表，避免频繁查询数据库
    mutable QList<Task> m_cachedTasks;
    mutable QMutex m_cacheMutex;
};

#endif // TASKMODEL_H
