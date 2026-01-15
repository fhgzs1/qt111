#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QTextStream>
#include <QMenu>
#include <QCursor>
#include <QDebug>
#include <QItemSelectionModel>
#include <QStatusBar>
#include <QTimer>
#include <QApplication>
#include "dbmanager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_taskModel(nullptr)
    , m_reminderThread(nullptr)
{
    qDebug() << "MainWindow构造函数开始";
    ui->setupUi(this);

    // 立即显示窗口
    this->setWindowTitle("个人工作与任务管理系统 - 正在启动...");

    // 延迟初始化
    QTimer::singleShot(100, this, [this]() {
        this->initializeApplication();
    });

    qDebug() << "MainWindow构造函数结束";
}

void MainWindow::initializeApplication()
{
    qDebug() << "开始初始化应用程序...";

    try {
        // 1. 初始化数据库
        qDebug() << "正在初始化数据库...";
        DBManager* dbManager = DBManager::instance();

        if (!dbManager->initDatabase()) {
            qCritical() << "数据库初始化失败！";
            QMessageBox::critical(this, "数据库错误",
                                  "无法初始化数据库，请检查文件权限。\n"
                                  "部分功能将不可用。");
            initializeUIWithoutDatabase();
            return;
        }

        qDebug() << "数据库初始化成功";

        // 2. 初始化任务模型
        qDebug() << "正在初始化任务模型...";
        m_taskModel = new TaskModel(this);
        ui->tableView_Tasks->setModel(m_taskModel);

        // 设置列宽
        ui->tableView_Tasks->setColumnWidth(0, 200);
        ui->tableView_Tasks->setColumnWidth(1, 150);
        ui->tableView_Tasks->setColumnWidth(2, 80);
        ui->tableView_Tasks->setColumnWidth(3, 80);

        // 3. 连接信号
        connect(ui->tableView_Tasks->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &MainWindow::onSelectionChanged);
        connect(ui->tableView_Tasks, &QTableView::doubleClicked,
                this, &MainWindow::onTableDoubleClicked);

        if (m_taskModel) {
            connect(m_taskModel, &TaskModel::taskDataChanged,
                    this, &MainWindow::onTaskDataChanged);
        }

        // 4. 设置表单默认值
        ui->dateTimeEdit_Deadline->setDateTime(QDateTime::currentDateTime().addSecs(3600));
        ui->comboBox_Priority->setCurrentIndex(1);

        // 5. 延迟启动提醒线程
        QTimer::singleShot(2000, this, [this]() {
            if (m_taskModel) {
                m_reminderThread = new ReminderThread(this);
                connect(m_reminderThread, &ReminderThread::taskReminder,
                        this, &MainWindow::onTaskReminder);
                m_reminderThread->setTasks(m_taskModel->getAllTasks());
                m_reminderThread->start();
                qDebug() << "提醒线程已启动";
            }
        });

        // 6. 更新窗口标题
        this->setWindowTitle("个人工作与任务管理系统");

        // 7. 显示状态
        ui->statusbar->showMessage("就绪", 3000);

        qDebug() << "应用程序初始化完成";

    } catch (const std::exception& e) {
        qCritical() << "初始化异常：" << e.what();
        QMessageBox::critical(this, "初始化错误",
                              QString("应用程序初始化失败：\n%1").arg(e.what()));
        initializeUIWithoutDatabase();
    } catch (...) {
        qCritical() << "未知初始化异常";
        QMessageBox::critical(this, "初始化错误", "应用程序初始化失败");
        initializeUIWithoutDatabase();
    }
}

void MainWindow::initializeUIWithoutDatabase()
{
    qDebug() << "初始化无数据库UI";
    this->setWindowTitle("个人工作与任务管理系统 (数据库不可用)");

    ui->btnAddTask->setEnabled(false);
    ui->btnEditTask->setEnabled(false);
    ui->btnDeleteTask->setEnabled(false);
    ui->btnRefresh->setEnabled(false);
    ui->btnStats->setEnabled(false);
    ui->btnExport->setEnabled(false);

    ui->tableView_Tasks->setEnabled(false);
    ui->tableView_Tasks->setToolTip("数据库不可用");

    ui->statusbar->showMessage("数据库不可用，功能受限", 5000);
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow析构函数开始";

    if (m_reminderThread) {
        qDebug() << "停止提醒线程...";
        m_reminderThread->stopThread();
        m_reminderThread->wait(1000);
        delete m_reminderThread;
        qDebug() << "提醒线程已停止";
    }

    delete ui;
    qDebug() << "MainWindow析构函数结束";
}

void MainWindow::on_btnAddTask_clicked()
{
    if (!m_taskModel) {
        QMessageBox::warning(this, "错误", "数据库不可用，无法添加任务");
        return;
    }

    QString title = ui->lineEdit_Title->text().trimmed();
    QDateTime deadline = ui->dateTimeEdit_Deadline->dateTime();
    int priority = ui->comboBox_Priority->currentIndex();
    QString description = ui->textEdit_Description->toPlainText().trimmed();

    if (title.isEmpty()) {
        QMessageBox::warning(this, "警告", "任务标题不能为空！");
        return;
    }
    if (deadline < QDateTime::currentDateTime()) {
        QMessageBox::warning(this, "警告", "截止时间不能早于当前时间！");
        return;
    }

    Task task;
    task.title = title;
    task.deadline = deadline;
    task.priority = priority;
    task.isCompleted = false;
    task.description = description;

    m_taskModel->addTask(task);

    clearInputForm();
    QMessageBox::information(this, "成功", "任务添加成功！");
}

void MainWindow::on_btnEditTask_clicked()
{
    if (!m_taskModel) {
        QMessageBox::warning(this, "错误", "数据库不可用，无法编辑任务");
        return;
    }

    int taskId = getSelectedTaskId();
    if (taskId == -1) {
        QMessageBox::warning(this, "警告", "请先选中要编辑的任务！");
        return;
    }

    QString title = ui->lineEdit_Title->text().trimmed();
    QDateTime deadline = ui->dateTimeEdit_Deadline->dateTime();
    int priority = ui->comboBox_Priority->currentIndex();
    QString description = ui->textEdit_Description->toPlainText().trimmed();

    if (title.isEmpty()) {
        QMessageBox::warning(this, "警告", "任务标题不能为空！");
        return;
    }
    if (deadline < QDateTime::currentDateTime()) {
        QMessageBox::warning(this, "警告", "截止时间不能早于当前时间！");
        return;
    }

    Task task = m_taskModel->getTaskById(taskId);
    if (task.id == -1) {
        QMessageBox::warning(this, "错误", "找不到选中的任务");
        return;
    }

    task.title = title;
    task.deadline = deadline;
    task.priority = priority;
    task.description = description;

    m_taskModel->updateTask(task);

    QMessageBox::information(this, "成功", "任务编辑成功！");
    clearInputForm();
}

void MainWindow::on_btnDeleteTask_clicked()
{
    if (!m_taskModel) {
        QMessageBox::warning(this, "错误", "数据库不可用，无法删除任务");
        return;
    }

    int taskId = getSelectedTaskId();
    if (taskId == -1) {
        QMessageBox::warning(this, "警告", "请先选中要删除的任务！");
        return;
    }

    if (QMessageBox::question(this, "确认", "确定要删除该任务吗？",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    m_taskModel->removeTask(taskId);
    QMessageBox::information(this, "成功", "任务删除成功！");
    clearInputForm();
}

void MainWindow::on_btnRefresh_clicked()
{
    if (!m_taskModel) {
        QMessageBox::warning(this, "错误", "数据库不可用，无法刷新");
        return;
    }

    ui->statusbar->showMessage("正在刷新任务列表...");
    ui->btnRefresh->setEnabled(false);
    QApplication::processEvents();

    m_taskModel->refreshTasks();

    ui->btnRefresh->setEnabled(true);
    QApplication::processEvents();
    QMessageBox::information(this, "提示", "任务列表已刷新！");
    ui->statusbar->showMessage("刷新完成", 2000);
}

void MainWindow::on_btnStats_clicked()
{
    if (!m_taskModel) {
        QMessageBox::warning(this, "错误", "数据库不可用，无法统计");
        return;
    }

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

    QString completionRate = (total > 0) ?
                                 QString::number((completed * 100.0) / total, 'f', 1) : "0.0";

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

void MainWindow::on_btnExport_clicked()
{
    if (!m_taskModel) {
        QMessageBox::warning(this, "错误", "数据库不可用，无法导出");
        return;
    }

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
    if (!m_taskModel) return;

    QString fileName = QFileDialog::getSaveFileName(this, "导出CSV",
                                                    QDir::homePath() + "/任务列表.csv",
                                                    "CSV文件 (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << "ID,标题,截止时间,优先级,完成状态,描述\n";
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
    } else {
        QMessageBox::warning(this, "错误", "无法创建文件：" + fileName);
    }
}

void MainWindow::exportToText()
{
    if (!m_taskModel) return;

    QString fileName = QFileDialog::getSaveFileName(this, "导出文本",
                                                    QDir::homePath() + "/任务列表.txt",
                                                    "文本文件 (*.txt)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << "任务列表报表\n";
        stream << "生成时间：" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") << "\n";
        stream << "=================================\n\n";
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
    if (m_reminderThread && m_taskModel) {
        m_reminderThread->setTasks(m_taskModel->getAllTasks());
    }
}

void MainWindow::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    if (selected.isEmpty()) {
        clearInputForm();
        return;
    }

    QModelIndex index = selected.indexes().first();
    if (!index.isValid()) {
        return;
    }

    int taskId = getSelectedTaskId();
    if (taskId == -1) {
        return;
    }

    if (!m_taskModel) return;
    Task task = m_taskModel->getTaskById(taskId);

    ui->lineEdit_Title->setText(task.title);
    ui->dateTimeEdit_Deadline->setDateTime(task.deadline);
    ui->comboBox_Priority->setCurrentIndex(task.priority);
    ui->textEdit_Description->setText(task.description);

    qDebug() << "选中任务：" << task.title;
}

void MainWindow::onTableDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid() || !m_taskModel) {
        return;
    }

    if (index.column() == TaskModel::ColumnCompleted) {
        int taskId = getSelectedTaskId();
        if (taskId != -1) {
            m_taskModel->toggleTaskCompleted(taskId);
            Task task = m_taskModel->getTaskById(taskId);
            QMessageBox::information(this, "提示",
                                     QString("任务'%1'状态已更新为：%2")
                                         .arg(task.title)
                                         .arg(task.isCompleted ? "已完成" : "未完成"));
        }
    }
}

int MainWindow::getSelectedTaskId() const
{
    if (!m_taskModel) return -1;

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
    if (ui->tableView_Tasks->selectionModel()) {
        ui->tableView_Tasks->clearSelection();
    }
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionAbout_triggered()
{
    QString dbPath = DBManager::instance()->getDatabasePath();
    QMessageBox::about(this, "关于",
                       "个人工作与任务管理系统 v1.0 (数据库版)\n"
                       "核心功能：\n"
                       "- 任务分类管理\n"
                       "- 优先级排序\n"
                       "- 定时提醒\n"
                       "- 完成情况统计\n"
                       "- SQLite本地数据库存储（数据持久化）\n"
                       "- 文件导出功能\n\n"
                       "数据库文件：\n" + dbPath);
}
