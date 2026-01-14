#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QPushButton>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QLabel>
#include <QMessageBox>
#include "taskmodel.h"
#include "reminderthread.h"
#include "dbmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 按钮点击事件
    void on_btnAddTask_clicked();    // 添加任务
    void on_btnEditTask_clicked();   // 编辑任务
    void on_btnDeleteTask_clicked(); // 删除任务
    void on_btnRefresh_clicked();    // 刷新任务列表
    void on_btnStats_clicked();      // 显示统计信息
    void on_btnExport_clicked();     // 导出文件

    // 菜单栏事件
    void on_actionExit_triggered();   // 退出程序（新增）
    void on_actionAbout_triggered();  // 关于程序（新增）

    // 其他槽函数
    void onTaskReminder(const Task &task); // 接收任务提醒
    void onTaskDataChanged();             // 任务数据变化（更新线程任务列表）
    void onDBError(const QString &errorMsg); // 接收数据库错误

private:
    Ui::MainWindow *ui;
    TaskModel *m_taskModel;
    ReminderThread *m_reminderThread;
    DBManager *m_dbManager;

    // 初始化UI控件
    void initUI();
    // 加载任务（从数据库到Model）
    void loadTasksFromDB();
    // 获取选中的任务ID
    int getSelectedTaskId() const;
    // 清空输入表单
    void clearInputForm();
    // 导出功能
    void exportToExcel();
    void exportToPDF();
};

#endif // MAINWINDOW_H
