#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QItemSelection>
#include "taskmodel.h"
#include "reminderthread.h"

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
    void on_actionExit_triggered();   // 退出程序
    void on_actionAbout_triggered();  // 关于程序
    // 其他槽函数
    void onTaskReminder(const Task &task); // 接收任务提醒
    void onTaskDataChanged();             // 任务数据变化（更新线程任务列表）
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onTableDoubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    TaskModel *m_taskModel;
    ReminderThread *m_reminderThread;
    // 辅助方法
    void loadTasks();                      // 加载任务（从数据库）
    int getSelectedTaskId() const;         // 获取选中的任务ID
    void clearInputForm();                 // 清空输入表单
    void exportToExcel();                  // 导出为CSV
    void exportToText();                   // 导出为文本
    void addSampleTasks();                 // 添加示例数据
};

#endif // MAINWINDOW_H
