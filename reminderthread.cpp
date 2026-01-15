#include "reminderthread.h"
#include <QDebug>
#include <QMutexLocker>

ReminderThread::ReminderThread(QObject *parent)
    : QThread(parent)
    , m_isRunning(true)
{}

ReminderThread::~ReminderThread()
{
    stopThread();
}

void ReminderThread::setTasks(const QList<Task> &tasks)
{
    QMutexLocker locker(&m_mutex); // 加锁，避免和run方法并发访问
    m_tasks = tasks;
}

void ReminderThread::stopThread()
{
    m_isRunning = false;
    if (isRunning()) {
        wait(); // 等待线程退出
    }
}

void ReminderThread::run()
{
    while (m_isRunning) {
        QDateTime now = QDateTime::currentDateTime();
        QList<Task> currentTasks;

        // 加锁读取当前任务列表（避免和setTasks并发修改）
        {
            QMutexLocker locker(&m_mutex);
            currentTasks = m_tasks;
        }

        // 遍历任务检查是否需要提醒
        for (const Task &task : currentTasks) {
            if (task.isCompleted) continue; // 已完成任务跳过

            // 计算剩余时间（秒）
            qint64 remaining = now.secsTo(task.deadline);
            if (remaining >= 0 && remaining <= REMINDER_ADVANCE) {
                // 发送提醒信号（注意：跨线程信号槽自动队列调度）
                emit taskReminder(task);
                // 避免重复提醒（标记为“已提醒”，但当前是内存版，可临时跳过）
                msleep(REMINDER_ADVANCE * 1000); // 避免1分钟内重复提醒
            }
        }

        msleep(CHECK_INTERVAL); // 每秒检查一次
    }
    qDebug() << "提醒线程已退出";
}
