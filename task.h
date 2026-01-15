#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QDateTime>

// 任务结构体（与数据库表字段对应）
struct Task {
    int id;                 // 唯一ID（数据库自增）
    QString title;          // 任务标题
    QDateTime deadline;     // 截止时间
    int priority;           // 优先级（0=低，1=中，2=高）
    bool isCompleted;       // 是否完成
    QString description;    // 任务描述
};

#endif // TASK_H
