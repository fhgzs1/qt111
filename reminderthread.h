#ifndef REMINDERTHREAD_H
#define REMINDERTHREAD_H

#include <QThread>
#include <QList>
#include <QDateTime>
#include <QMutex>
#include "task.h"  // 包含Task结构体

class ReminderThread : public QThread
{
    Q_OBJECT
public:
    explicit ReminderThread(QObject *parent = nullptr);
    ~ReminderThread() override;

    void setTasks(const QList<Task> &tasks);
    void stopThread();

signals:
    void taskReminder(const Task &task);

protected:
    void run() override;

private:
    QList<Task> m_tasks;
    volatile bool m_isRunning;
    mutable QMutex m_mutex;
    const int CHECK_INTERVAL = 1000;
    const int REMINDER_ADVANCE = 60;
};

#endif // REMINDERTHREAD_H
