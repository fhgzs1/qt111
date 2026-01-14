#include "reminderthread.h"
#include <QThread>

ReminderThread::ReminderThread(QObject *parent)
    : QThread(parent)
    , m_isRunning(true)
{}

ReminderThread::~ReminderThread()
{
    stopThread();
    wait();
}

void ReminderThread::setTasks(const QList<Task> &tasks)
{
    // 线程安全更新任务列表
    QMutexLocker locker(&m_mutex);
    m_tasks = tasks;
}

void ReminderThread::stopThread()
{
    m_isRunning = false;
}

void ReminderThread::run()
{
    while (m_isRunning) {
        QDateTime now = QDateTime::currentDateTime();
        QList<Task> tasksCopy;

        // 先复制任务列表，减少锁持有时间
        {
            QMutexLocker locker(&m_mutex);
            tasksCopy = m_tasks;
        }

        // 遍历任务，检查是否需要提醒
        for (const auto &task : tasksCopy) {
            if (task.isCompleted) continue; // 已完成任务跳过

            // 计算截止时间与当前时间的差值（秒）
            qint64 secsToDeadline = now.secsTo(task.deadline);
            // 到期前60秒内且未提醒过，发送提醒
            if (secsToDeadline >= 0 && secsToDeadline <= REMINDER_ADVANCE) {
                emit taskReminder(task);
                // 避免重复提醒（短暂延迟后跳过该任务）
                QThread::msleep(100);
            }
        }

        QThread::msleep(CHECK_INTERVAL); // 每秒检查一次
    }
}
