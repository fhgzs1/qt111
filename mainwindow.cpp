// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QTextStream>
#include <QMenu>
#include <QCursor>
#include <QDebug>
#include <QItemSelectionModel>
#include "dbmanager.h"
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // 先初始化UI
    ui->setupUi(this);

    // 显示当前数据库路径（状态栏）
    QString dbPath = DBManager::instance()->getDatabasePath();
    ui->statusbar->showMessage("数据库文件: " + dbPath);

    // 初始化数据库（添加详细的调试信息）
    qDebug() << "开始初始化数据库...";
    qDebug() << "数据库文件路径：" << dbPath;

    DBManager* dbManager = DBManager::instance();
    qDebug() << "DBManager实例地址：" << dbManager;

    if (!dbManager->initDatabase()) {
        qCritical() << "数据库初始化失败！";

        // 显示具体错误
        QString errorMsg = QString("数据库初始化失败！\n"
                                   "文件路径：%1\n"
                                   "请检查：\n"
                                   "1. 磁盘空间是否足够\n"
                                   "2. 是否有文件写入权限\n"
                                   "3. 文件是否被其他程序占用").arg(dbPath);

        QMessageBox::critical(this, "错误", errorMsg);

        // 不退出程序，但禁用相关功能
        ui->btnAddTask->setEnabled(false);
        ui->btnEditTask->setEnabled(false);
        ui->btnDeleteTask->setEnabled(false);
        ui->btnRefresh->setEnabled(false);
        ui->btnStats->setEnabled(false);
        ui->btnExport->setEnabled(false);
    } else {
        qDebug() << "数据库初始化成功！";
        // 检查数据库连接状态
        if (dbManager->isDatabaseOpen()) {
            qDebug() << "数据库连接正常打开";
            ui->statusbar->showMessage("数据库连接正常", 3000);
        } else {
            qWarning() << "数据库连接未打开";
            ui->statusbar->showMessage("数据库未连接", 3000);
        }
    }

    // 初始化任务Model
    m_taskModel = new TaskModel(this);
    ui->tableView_Tasks->setModel(m_taskModel);

    // 设置列宽（优化显示）
    ui->tableView_Tasks->setColumnWidth(TaskModel::ColumnTitle, 200);
    ui->tableView_Tasks->setColumnWidth(TaskModel::ColumnDeadline, 150);
    ui->tableView_Tasks->setColumnWidth(TaskModel::ColumnPriority, 80);
    ui->tableView_Tasks->setColumnWidth(TaskModel::ColumnCompleted, 80);

    // 双击编辑，单行选择
    ui->tableView_Tasks->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->tableView_Tasks->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_Tasks->setSelectionMode(QAbstractItemView::SingleSelection);

    // 连接选择变化信号
    connect(ui->tableView_Tasks->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);

    // 连接双击信号，用于切换完成状态
    connect(ui->tableView_Tasks, &QTableView::doubleClicked,
            this, &MainWindow::onTableDoubleClicked);

    // 加载示例任务（仅首次启动时添加）
    loadTasks();

    // 初始化提醒线程
    m_reminderThread = new ReminderThread(this);
    connect(m_reminderThread, &ReminderThread::taskReminder, this, &MainWindow::onTaskReminder);
    connect(m_taskModel, &TaskModel::taskDataChanged, this, &MainWindow::onTaskDataChanged);

    // 设置初始任务列表
    m_reminderThread->setTasks(m_taskModel->getAllTasks());

    // 启动线程
    m_reminderThread->start();

    // 初始化输入表单默认值（当前时间+1小时）
    ui->dateTimeEdit_Deadline->setDateTime(QDateTime::currentDateTime().addSecs(3600));
    ui->comboBox_Priority->setCurrentIndex(1); // 默认中等优先级

    // 激活窗口
    this->raise();
    this->activateWindow();

    qDebug() << "程序启动完成，数据库模式";
    qDebug() << "当前任务数量：" << m_taskModel->rowCount();
}

MainWindow::~MainWindow()
{
    // 安全停止线程
    if (m_reminderThread) {
        m_reminderThread->stopThread();
        m_reminderThread->wait();
        delete m_reminderThread;
    }
    delete m_taskModel;
    delete ui;
}

void MainWindow::loadTasks()
{
    // 加载示例任务（仅数据库为空时添加）
    addSampleTasks();
    qDebug() << "加载了" << m_taskModel->rowCount() << "个任务";
}

void MainWindow::addSampleTasks()
{
    QList<Task> tasks = m_taskModel->getAllTasks();
    if (!tasks.isEmpty()) return; // 已有数据则跳过

    // 示例任务1：即将到期的任务
    Task task1;
    task1.title = "完成项目报告";
    task1.deadline = QDateTime::currentDateTime().addSecs(300); // 5分钟后到期
    task1.priority = 2; // 高优先级
    task1.isCompleted = false;
    task1.description = "编写项目总结报告，包括数据分析部分";
    m_taskModel->addTask(task1);

    // 示例任务2：今天要完成的任务
    Task task2;
    task2.title = "团队会议";
    task2.deadline = QDateTime::currentDateTime().addSecs(7200); // 2小时后
    task2.priority = 1; // 中优先级
    task2.isCompleted = false;
    task2.description = "每周团队例会，讨论项目进展";
    m_taskModel->addTask(task2);

    // 示例任务3：明天的任务
    Task task3;
    task3.title = "购买办公用品";
    task3.deadline = QDateTime::currentDateTime().addDays(1).addSecs(36000); // 明天上午10点
    task3.priority = 0; // 低优先级
    task3.isCompleted = false;
    task3.description = "购买打印机墨盒和纸张";
    m_taskModel->addTask(task3);

    // 示例任务4：已完成的任务
    Task task4;
    task4.title = "提交周报";
    task4.deadline = QDateTime::currentDateTime().addSecs(-3600); // 1小时前已到期
    task4.priority = 1; // 中优先级
    task4.isCompleted = true;
    task4.description = "提交上周工作总结和本周计划";
    m_taskModel->addTask(task4);

    // 示例任务5：下周的任务
    Task task5;
    task5.title = "学习Qt高级特性";
    task5.deadline = QDateTime::currentDateTime().addDays(7); // 一周后
    task5.priority = 0; // 低优先级
    task5.isCompleted = false;
    task5.description = "学习Qt模型/视图架构和多线程编程";
    m_taskModel->addTask(task5);
}

void MainWindow::on_btnAddTask_clicked()
{
    // 获取表单输入
    QString title = ui->lineEdit_Title->text().trimmed();
    QDateTime deadline = ui->dateTimeEdit_Deadline->dateTime();
    int priority = ui->comboBox_Priority->currentIndex();
    QString description = ui->textEdit_Description->toPlainText().trimmed();

    // 输入校验
    if (title.isEmpty()) {
        QMessageBox::warning(this, "警告", "任务标题不能为空！");
        return;
    }
    if (deadline < QDateTime::currentDateTime()) {
        QMessageBox::warning(this, "警告", "截止时间不能早于当前时间！");
        return;
    }

    // 构造任务对象（ID由数据库自增）
    Task task;
    task.title = title;
    task.deadline = deadline;
    task.priority = priority;
    task.isCompleted = false;
    task.description = description;

    // 添加到数据库
    m_taskModel->addTask(task);
    QMessageBox::information(this, "成功", "任务添加成功！");
    qDebug() << "添加新任务 - 标题:" << task.title;
    qDebug() << "当前任务总数:" << m_taskModel->rowCount();
    clearInputForm();
}

void MainWindow::on_btnEditTask_clicked()
{
    int taskId = getSelectedTaskId();
    if (taskId == -1) {
        QMessageBox::warning(this, "警告", "请先选中要编辑的任务！");
        return;
    }

    // 获取表单输入
    QString title = ui->lineEdit_Title->text().trimmed();
    QDateTime deadline = ui->dateTimeEdit_Deadline->dateTime();
    int priority = ui->comboBox_Priority->currentIndex();
    QString description = ui->textEdit_Description->toPlainText().trimmed();

    // 输入校验
    if (title.isEmpty()) {
        QMessageBox::warning(this, "警告", "任务标题不能为空！");
        return;
    }
    if (deadline < QDateTime::currentDateTime()) {
        QMessageBox::warning(this, "警告", "截止时间不能早于当前时间！");
        return;
    }

    // 获取原任务并更新
    Task task = m_taskModel->getTaskById(taskId);
    task.title = title;
    task.deadline = deadline;
    task.priority = priority;
    task.description = description;

    // 更新到数据库
    m_taskModel->updateTask(task);
    QMessageBox::information(this, "成功", "任务编辑成功！");
    qDebug() << "编辑任务 - ID:" << task.id << "新标题:" << task.title;
    clearInputForm();
}

void MainWindow::on_btnDeleteTask_clicked()
{
    int taskId = getSelectedTaskId();
    if (taskId == -1) {
        QMessageBox::warning(this, "警告", "请先选中要删除的任务！");
        return;
    }

    if (QMessageBox::question(this, "确认", "确定要删除该任务吗？",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // 从数据库删除
    m_taskModel->removeTask(taskId);
    QMessageBox::information(this, "成功", "任务删除成功！");
    qDebug() << "删除任务 - ID:" << taskId;
    qDebug() << "剩余任务数:" << m_taskModel->rowCount();
    clearInputForm();
}

void MainWindow::on_btnRefresh_clicked()
{
    m_taskModel->refreshTasks();
    QMessageBox::information(this, "提示", "任务列表已刷新！");
    qDebug() << "刷新任务列表，当前任务数:" << m_taskModel->rowCount();
}

void MainWindow::on_btnStats_clicked()
{
    QList<Task> allTasks = m_taskModel->getAllTasks();
    int total = allTasks.size();
    int completed = 0;
    int highPriority = 0;
    int upcoming = 0;
    QDateTime now = QDateTime::currentDateTime();

    for (const auto &task : allTasks) {
        if (task.isCompleted) completed++;
        if (task.priority == 2) highPriority++;
        if (!task.isCompleted && task.deadline > now) upcoming++;
    }

    // 计算完成率
    QString completionRate = (total > 0) ?
                                 QString::number((completed * 100.0) / total, 'f', 1) : "0.0";

    // 显示统计结果
    QString statsText = QString(
                            "任务统计\n"
                            "----------------\n"
                            "总任务数：%1\n"
                            "已完成：%2（%3%）\n"
                            "高优先级：%4\n"
                            "待完成（未到期）：%5"
                            ).arg(total)
                            .arg(completed)
                            .arg(completionRate)
                            .arg(highPriority)
                            .arg(upcoming);

    QMessageBox::information(this, "任务统计", statsText);
    qDebug() << "统计信息 - 总数:" << total << "已完成:" << completed;
}

void MainWindow::on_btnExport_clicked()
{
    QMenu menu(this);
    QAction *csvAction = menu.addAction("导出为CSV");
    QAction *textAction = menu.addAction("导出为文本");
    QAction *selected = menu.exec(QCursor::pos());

    if (selected == csvAction) {
        exportToExcel();
    } else if (selected == textAction) {
        exportToText();
    }
}

void MainWindow::exportToExcel()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出CSV",
                                                    QDir::homePath() + "/任务列表.csv",
                                                    "CSV文件 (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        // 写入表头
        stream << "ID,标题,截止时间,优先级,完成状态,描述\n";
        // 写入数据
        QList<Task> tasks = m_taskModel->getAllTasks();
        for (const auto &task : tasks) {
            stream << task.id << ","
                   << "\"" << task.title << "\","
                   << task.deadline.toString("yyyy-MM-dd HH:mm") << ","
                   << task.priority << ","
                   << (task.isCompleted ? "已完成" : "未完成") << ","
                   << "\"" << task.description << "\"\n";
        }
        file.close();
        QMessageBox::information(this, "成功", "数据已导出到：" + fileName);
        qDebug() << "导出" << tasks.size() << "个任务到:" << fileName;
    } else {
        QMessageBox::warning(this, "错误", "无法创建文件：" + fileName);
    }
}

void MainWindow::exportToText()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出文本",
                                                    QDir::homePath() + "/任务列表.txt",
                                                    "文本文件 (*.txt)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        // 写入标题
        stream << "任务列表报表\n";
        stream << "生成时间：" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") << "\n";
        stream << "=================================\n\n";
        // 写入任务数据
        QList<Task> tasks = m_taskModel->getAllTasks();
        for (const auto &task : tasks) {
            stream << "ID: " << task.id << "\n";
            stream << "标题: " << task.title << "\n";
            stream << "截止时间: " << task.deadline.toString("yyyy-MM-dd HH:mm") << "\n";
            stream << "优先级: " << (task.priority == 0 ? "低" : (task.priority == 1 ? "中" : "高")) << "\n";
            stream << "完成状态: " << (task.isCompleted ? "已完成" : "未完成") << "\n";
            if (!task.description.isEmpty()) {
                stream << "描述: " << task.description << "\n";
            }
            stream << "---------------------------------\n";
        }
        file.close();
        QMessageBox::information(this, "成功", "数据已导出到：" + fileName);
        qDebug() << "导出文本报告到:" << fileName;
    } else {
        QMessageBox::warning(this, "错误", "无法创建文件：" + fileName);
    }
}

void MainWindow::onTaskReminder(const Task &task)
{
    qDebug() << "任务提醒 - ID:" << task.id << "标题:" << task.title;
    QMessageBox::information(this, "任务提醒",
                             QString("任务即将到期！\n标题：%1\n截止时间：%2")
                                 .arg(task.title)
                                 .arg(task.deadline.toString("yyyy-MM-dd HH:mm")));
}

void MainWindow::onTaskDataChanged()
{
    qDebug() << "任务数据变化，更新提醒线程...";
    m_reminderThread->setTasks(m_taskModel->getAllTasks());
}

void MainWindow::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    if (selected.isEmpty()) {
        // 没有选中任何行，清空表单
        clearInputForm();
        return;
    }

    // 获取选中的行
    QModelIndex index = selected.indexes().first();
    if (!index.isValid()) {
        return;
    }

    // 获取任务ID
    int taskId = getSelectedTaskId();
    if (taskId == -1) {
        return;
    }

    // 获取任务信息
    Task task = m_taskModel->getTaskById(taskId);

    // 填充表单
    ui->lineEdit_Title->setText(task.title);
    ui->dateTimeEdit_Deadline->setDateTime(task.deadline);
    ui->comboBox_Priority->setCurrentIndex(task.priority);
    ui->textEdit_Description->setText(task.description);

    qDebug() << "选中任务：" << task.title;
}

void MainWindow::onTableDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    // 如果是完成状态列，切换完成状态
    if (index.column() == TaskModel::ColumnCompleted) {
        int taskId = getSelectedTaskId();
        if (taskId != -1) {
            // 切换完成状态
            m_taskModel->toggleTaskCompleted(taskId);
            // 获取更新后的任务
            Task task = m_taskModel->getTaskById(taskId);
            qDebug() << "切换任务完成状态：" << task.title << "->" << (task.isCompleted ? "已完成" : "未完成");
            QMessageBox::information(this, "提示",
                                     QString("任务'%1'状态已更新为：%2")
                                         .arg(task.title)
                                         .arg(task.isCompleted ? "已完成" : "未完成"));
        }
    } else {
        // 其他列，只是选中任务，表单已自动填充
        qDebug() << "双击行：" << index.row() << "列：" << index.column();
    }
}

int MainWindow::getSelectedTaskId() const
{
    QModelIndexList selectedRows = ui->tableView_Tasks->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) return -1;

    QModelIndex index = selectedRows.first();
    QList<Task> tasks = m_taskModel->getAllTasks();

    if (index.row() >= tasks.size()) return -1;
    return tasks[index.row()].id;
}

void MainWindow::clearInputForm()
{
    ui->lineEdit_Title->clear();
    ui->dateTimeEdit_Deadline->setDateTime(QDateTime::currentDateTime().addSecs(3600));
    ui->comboBox_Priority->setCurrentIndex(1);
    ui->textEdit_Description->clear();
    ui->tableView_Tasks->clearSelection();
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "关于",
                       "个人工作与任务管理系统 v1.0 (数据库版)\n"
                       "核心功能：\n"
                       "- 任务分类管理\n"
                       "- 优先级排序\n"
                       "- 定时提醒\n"
                       "- 完成情况统计\n"
                       "- SQLite本地数据库存储（数据持久化）\n"
                       "- 文件导出功能\n\n"
                       "数据库文件：\n" + DBManager::instance()->getDatabasePath());
}
