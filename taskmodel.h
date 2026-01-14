#ifndef TASKMODEL_H
#define TASKMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QMutex>

// 任务结构体（与数据库表字段对应）
struct Task {
    int id;                 // 唯一ID（自增）
    QString title;          // 任务标题
    QDateTime deadline;     // 截止时间
    int priority;           // 优先级（0=低，1=中，2=高）
    bool isCompleted;       // 是否完成
    QString description;    // 任务描述
};

class TaskModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    // 表格列定义
    enum TaskColumn {
        ColumnTitle,        // 任务标题
        ColumnDeadline,     // 截止时间
        ColumnPriority,     // 优先级
        ColumnCompleted,    // 完成状态
        ColumnCount         // 列数
    };

    explicit TaskModel(QObject *parent = nullptr);

    // 重写Model核心方法
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // 任务管理接口
    void addTask(const Task &task);          // 添加任务
    void updateTask(const Task &task);       // 更新任务
    void removeTask(int taskId);             // 删除任务
    void toggleTaskCompleted(int taskId);    // 切换任务完成状态
    void clearTasks();                       // 清空所有任务（新增）
    QList<Task> getAllTasks() const;         // 获取所有任务
    Task getTaskById(int taskId) const;      // 按ID获取任务

signals:
    void taskDataChanged();  // 任务数据变化信号

private:
    QList<Task> m_tasks;     // 任务列表（内存缓存）
    mutable QMutex m_mutex;  // 线程安全锁
    // 优先级转文字（低/中/高）
    QString priorityToString(int priority) const;
    // 截止时间格式化（yyyy-MM-dd HH:mm）
    QString formatDeadline(const QDateTime &deadline) const;
};

#endif // TASKMODEL_H
