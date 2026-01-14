#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// 核心头文件（解决所有未定义错误）
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QThread>
#include <QFile>
#include <QFileDialog>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QDataStream>
#include <QListWidgetItem>
// 关键修改：替换 QRegExp 为 QRegularExpression
#include <QRegularExpression>
#include <QDir>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 文件传输线程类（避免UI阻塞）
class FileTransferThread : public QThread
{
    Q_OBJECT
public:
    explicit FileTransferThread(QTcpSocket *socket, const QString &filePath, QObject *parent = nullptr);
    void run() override;

signals:
    void progressChanged(int value);
    void transferFinished(bool success);

private:
    QTcpSocket *m_socket;
    QString m_filePath;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 设备发现相关
    void discoverDevices();
    void readUdpDatagram();
    void on_listWidget_devices_itemClicked(QListWidgetItem *item);

    // 文件操作相关
    void on_pushButton_selectFile_clicked();
    void on_pushButton_sendFile_clicked();
    void updateProgress(int value);
    void transferDone(bool success);

    // 服务器相关（接收文件）
    void newClientConnected();
    void readClientData();

private:
    Ui::MainWindow *ui;

    // 网络相关
    QTcpServer *m_tcpServer;
    QTcpSocket *m_clientSocket;
    QUdpSocket *m_udpSocket;
    QString m_selectedDeviceIp;
    QString m_selectedFilePath;

    // 文件传输相关
    QFile *m_receiveFile;
    qint64 m_fileSize;
    qint64 m_receivedSize;
    FileTransferThread *m_transferThread;

    // 初始化函数
    void initNetwork();
    void initUI();
};

#endif // MAINWINDOW_H
