#ifndef REMINDERTHREAD_H
#define REMINDERTHREAD_H
#include <QThread>
#include <QList>
#include <QDateTime>
#include "taskmodel.h"
#include <QMutex>

class ReminderThread : public QThread
{
    Q_OBJECT
public:
    explicit ReminderThread(QObject *parent = nullptr);
    ~ReminderThread() override;

    void setTasks(const QList<Task> &tasks); // 设置待监控任务
    void stopThread();                       // 停止线程

signals:
    void taskReminder(const Task &task);     // 任务到期提醒信号

protected:
    void run() override;                     // 线程核心逻辑

private:
    QList<Task> m_tasks;
    volatile bool m_isRunning; // 加volatile，确保多线程可见性
    mutable QMutex m_mutex;                  // 线程安全锁
    const int CHECK_INTERVAL = 1000;         // 任务检查间隔（1秒）
    const int REMINDER_ADVANCE = 60;         // 提前提醒时间（60秒）
};

#endif // REMINDERTHREAD_H
