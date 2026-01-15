#include "reminderthread.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>

ReminderThread::ReminderThread(QObject *parent)
    : QThread(parent)
    , m_isRunning(true)
{
    qDebug() << "ReminderThread构造函数";
}

ReminderThread::~ReminderThread()
{
    qDebug() << "ReminderThread析构函数";
    stopThread();
}

void ReminderThread::setTasks(const QList<Task> &tasks)
{
    QMutexLocker locker(&m_mutex);
    m_tasks = tasks;
}

void ReminderThread::stopThread()
{
    qDebug() << "请求停止线程";
    m_isRunning = false;
}

void ReminderThread::run()
{
    qDebug() << "提醒线程开始运行";

    int checkCounter = 0;

    while (m_isRunning) {
        // 每5秒检查一次
        if (checkCounter % 5 == 0) {
            QDateTime now = QDateTime::currentDateTime();
            QList<Task> currentTasks;

            {
                QMutexLocker locker(&m_mutex);
                currentTasks = m_tasks;
            }

            for (const Task &task : currentTasks) {
                if (task.isCompleted) continue;

                qint64 remaining = now.secsTo(task.deadline);
                if (remaining >= 0 && remaining <= 60) {
                    qDebug() << "发送任务提醒:" << task.title;
                    emit taskReminder(task);
                }
            }
        }

        checkCounter++;

        if (!m_isRunning) break;

        msleep(1000);

        if (checkCounter > 86400) {
            checkCounter = 0;
        }
    }

    qDebug() << "提醒线程安全退出";
}
