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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 初始化单例数据库
    m_dbManager = DBManager::getInstance(this);
    if (!m_dbManager->initDB()) {
        QMessageBox::critical(this, "错误", "数据库初始化失败，程序将退出！");
        close();
        return;
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

    // 先加载任务
    loadTasksFromDB();

    // 初始化提醒线程
    m_reminderThread = new ReminderThread(this);
    connect(m_reminderThread, &ReminderThread::taskReminder, this, &MainWindow::onTaskReminder);
    connect(m_taskModel, &TaskModel::taskDataChanged, this, &MainWindow::onTaskDataChanged);

    // 设置初始任务列表
    m_reminderThread->setTasks(m_taskModel->getAllTasks());

    // 启动线程
    m_reminderThread->start();

    // 连接数据库错误信号
    connect(m_dbManager, &DBManager::dbError, this, &MainWindow::onDBError);

    // 初始化输入表单默认值（当前时间+1小时）
    ui->dateTimeEdit_Deadline->setDateTime(QDateTime::currentDateTime().addSecs(3600));
    ui->comboBox_Priority->setCurrentIndex(1); // 默认中等优先级

    // 连接导出按钮
    connect(ui->btnExport, &QPushButton::clicked, this, &MainWindow::on_btnExport_clicked);

    // 注意：菜单栏的 action 已经在 ui 文件中通过名称自动连接了
    // 不需要手动 connect，Qt 的自动连接机制会自动连接名为 on_actionName_triggered 的槽函数
}

MainWindow::~MainWindow()
{
    // 安全停止线程
    m_reminderThread->stopThread();
    m_reminderThread->wait();
    // 释放资源
    delete m_reminderThread;
    delete m_taskModel;
    delete ui;
}

// 添加任务
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

    // 构造任务对象
    Task task;
    task.id = m_dbManager->getNextTaskId();
    task.title = title;
    task.deadline = deadline;
    task.priority = priority;
    task.isCompleted = false;
    task.description = description;

    // 添加到数据库和Model
    if (m_dbManager->addTask(task)) {
        m_taskModel->addTask(task);
        QMessageBox::information(this, "成功", "任务添加成功！");
        clearInputForm();
    }
}

// 编辑任务
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

    // 构造更新后的任务
    Task task = m_taskModel->getTaskById(taskId);
    task.title = title;
    task.deadline = deadline;
    task.priority = priority;
    task.description = description;

    // 更新数据库和Model
    if (m_dbManager->updateTask(task)) {
        m_taskModel->updateTask(task);
        QMessageBox::information(this, "成功", "任务编辑成功！");
        clearInputForm();
    }
}

// 删除任务
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

    // 从数据库和Model删除
    if (m_dbManager->deleteTask(taskId)) {
        m_taskModel->removeTask(taskId);
        QMessageBox::information(this, "成功", "任务删除成功！");
        clearInputForm();
    }
}

// 刷新任务列表
void MainWindow::on_btnRefresh_clicked()
{
    // 修复：直接调用 loadTasksFromDB()，它内部会调用 clearTasks()
    loadTasksFromDB();
    QMessageBox::information(this, "提示", "任务列表已刷新！");
}

// 显示统计信息
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

    // 计算完成率（避免除以0）
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
}

// 导出文件
void MainWindow::on_btnExport_clicked()
{
    QMenu menu(this);
    QAction *excelAction = menu.addAction("导出为Excel");
    QAction *pdfAction = menu.addAction("导出为PDF");

    QAction *selected = menu.exec(QCursor::pos());
    if (selected == excelAction) {
        exportToExcel();
    } else if (selected == pdfAction) {
        exportToPDF();
    }
}

// 导出为Excel
void MainWindow::exportToExcel()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出Excel",
                                                    QDir::homePath() + "/任务统计.xlsx",
                                                    "Excel文件 (*.xlsx)");
    if (fileName.isEmpty()) return;

    // 简单实现：导出为CSV格式
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
// 简化编码设置
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        stream.setEncoding(QStringConverter::Utf8);
#else
        stream.setCodec("UTF-8");
#endif

        // 写入表头
        stream << "ID,标题,截止时间,优先级,完成状态,描述\n";

        // 写入数据
        QList<Task> tasks = m_taskModel->getAllTasks();
        for (const auto &task : tasks) {
            stream << task.id << ","
                   << task.title << ","
                   << task.deadline.toString("yyyy-MM-dd HH:mm") << ","
                   << task.priority << ","
                   << (task.isCompleted ? "已完成" : "未完成") << ","
                   << task.description << "\n";
        }

        file.close();
        QMessageBox::information(this, "成功", "数据已导出到：" + fileName);
    } else {
        QMessageBox::warning(this, "错误", "无法创建文件：" + fileName);
    }
}

// 导出为PDF
void MainWindow::exportToPDF()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出PDF",
                                                    QDir::homePath() + "/任务统计.pdf",
                                                    "PDF文件 (*.pdf)");
    if (fileName.isEmpty()) return;

    // 简单实现：先导出为文本文件
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
// 简化编码设置
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        stream.setEncoding(QStringConverter::Utf8);
#else
        stream.setCodec("UTF-8");
#endif

        // 写入标题
        stream << "任务统计报表\n";
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
    } else {
        QMessageBox::warning(this, "错误", "无法创建文件：" + fileName);
    }
}

// 接收任务提醒（UI线程显示弹窗）
void MainWindow::onTaskReminder(const Task &task)
{
    // 线程安全更新UI（队列连接）
    QMetaObject::invokeMethod(this, [=]() {
        QMessageBox::information(this, "任务提醒",
                                 QString("任务即将到期！\n标题：%1\n截止时间：%2")
                                     .arg(task.title)
                                     .arg(task.deadline.toString("yyyy-MM-dd HH:mm")));
    }, Qt::QueuedConnection);
}

// 任务数据变化（更新线程任务列表）
void MainWindow::onTaskDataChanged()
{
    m_reminderThread->setTasks(m_taskModel->getAllTasks());
}

// 接收数据库错误
void MainWindow::onDBError(const QString &errorMsg)
{
    QMessageBox::critical(this, "数据库错误", errorMsg);
}

// 加载任务（从数据库到Model）
void MainWindow::loadTasksFromDB()
{
    QList<Task> tasks = m_dbManager->getAllTasks();

    // 清空Model并重新添加任务
    m_taskModel->clearTasks();  // 这会调用 beginResetModel() 和 endResetModel()
    for (const auto &task : tasks) {
        m_taskModel->addTask(task);
    }

    // 更新提醒线程的任务列表
    m_reminderThread->setTasks(tasks);
}

// 获取选中的任务ID
int MainWindow::getSelectedTaskId() const
{
    QModelIndexList selectedRows = ui->tableView_Tasks->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) return -1;

    QModelIndex index = selectedRows.first();
    QList<Task> tasks = m_taskModel->getAllTasks();
    if (index.row() >= tasks.size()) return -1;

    return tasks[index.row()].id;
}

// 清空输入表单
void MainWindow::clearInputForm()
{
    ui->lineEdit_Title->clear();
    ui->dateTimeEdit_Deadline->setDateTime(QDateTime::currentDateTime().addSecs(3600));
    ui->comboBox_Priority->setCurrentIndex(1);
    ui->textEdit_Description->clear();
    ui->tableView_Tasks->clearSelection();
}

// 退出程序 - 菜单栏Action
void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

// 显示关于信息 - 菜单栏Action
void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "关于",
                       "个人工作与任务管理系统 v1.0\n"
                       "核心功能：\n"
                       "- 任务分类管理\n"
                       "- 优先级排序\n"
                       "- 定时提醒\n"
                       "- 完成情况统计\n"
                       "- 数据库存储\n"
                       "- 文件导出功能");
}
