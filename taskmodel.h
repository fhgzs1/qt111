#ifndef TASKMODEL_H
#define TASKMODEL_H

#include <QAbstractTableModel>
#include "dbmanager.h"
#include "task.h"

class TaskModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum TaskColumn {
        ColumnTitle = 0,
        ColumnDeadline,
        ColumnPriority,
        ColumnCompleted,
        ColumnCount
    };

    explicit TaskModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

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
    QList<Task> m_cachedTasks;
};

#endif // TASKMODEL_H
