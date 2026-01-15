#ifndef REMINDERTHREAD_H
#define REMINDERTHREAD_H

#include <QThread>
#include <QList>
#include <QDateTime>
#include <QMutex>
#include "task.h"

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
};

#endif // REMINDERTHREAD_H
