#include "mainwindow.h"
#include "ui_mainwindow.h"

// 文件传输线程实现
FileTransferThread::FileTransferThread(QTcpSocket *socket, const QString &filePath, QObject *parent)
    : QThread(parent), m_socket(socket), m_filePath(filePath)
{}

void FileTransferThread::run()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit transferFinished(false);
        return;
    }

    qint64 fileSize = file.size();
    qint64 sentSize = 0;

    // 先发送文件大小和文件名
    QByteArray header;
    QDataStream headerStream(&header, QIODevice::WriteOnly);
    // 适配Qt 6：使用当前Qt版本的DataStream版本（无需固定Qt_5_15）
    headerStream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    headerStream << fileSize << QFileInfo(m_filePath).fileName();
    m_socket->write(header);
    m_socket->flush();

    // 分块发送文件内容
    const qint64 blockSize = 4096;
    char buffer[blockSize];
    while (sentSize < fileSize) {
        qint64 readSize = file.read(buffer, qMin(blockSize, fileSize - sentSize));
        if (readSize <= 0) break;

        qint64 writeSize = m_socket->write(buffer, readSize);
        if (writeSize <= 0) break;

        sentSize += writeSize;
        emit progressChanged((int)((sentSize * 100) / fileSize));
        m_socket->flush();
    }

    file.close();
    emit transferFinished(sentSize == fileSize);
}

// 主窗口实现
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_tcpServer(new QTcpServer(this))
    , m_clientSocket(nullptr)
    , m_udpSocket(new QUdpSocket(this))
    , m_receiveFile(nullptr)
    , m_fileSize(0)
    , m_receivedSize(0)
    , m_transferThread(nullptr)
{
    ui->setupUi(this);
    initUI();
    initNetwork();
}

MainWindow::~MainWindow()
{
    // 安全停止线程
    if (m_transferThread && m_transferThread->isRunning()) {
        m_transferThread->quit();
        m_transferThread->wait();
    }
    // 清理文件对象
    if (m_receiveFile) {
        m_receiveFile->close();
        delete m_receiveFile;
    }
    delete ui;
}

void MainWindow::initUI()
{
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
}

void MainWindow::initNetwork()
{
    // 启动TCP服务器（监听端口8888）
    if (!m_tcpServer->listen(QHostAddress::Any, 8888)) {
        QMessageBox::warning(this, "警告", "TCP服务器启动失败：" + m_tcpServer->errorString());
    } else {
        connect(m_tcpServer, &QTcpServer::newConnection, this, &MainWindow::newClientConnected);
    }

    // 初始化UDP（设备发现，端口9999）
    m_udpSocket->bind(QHostAddress::Any, 9999, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readUdpDatagram);

    // 自动发现局域网设备
    discoverDevices();
}

// 发现局域网设备
void MainWindow::discoverDevices()
{
    ui->listWidget_devices->clear();

    // 广播设备发现请求
    QByteArray discoverMsg = "LAN_FILE_TRANSFER_DISCOVER";
    m_udpSocket->writeDatagram(discoverMsg, QHostAddress::Broadcast, 9999);

    // 获取本机IP并添加到设备列表
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        if (interface.flags() & QNetworkInterface::IsUp && !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    QString ip = entry.ip().toString();
                    ui->listWidget_devices->addItem(new QListWidgetItem(QString("本机 (%1)").arg(ip), ui->listWidget_devices));
                }
            }
        }
    }
}

// 读取UDP数据报（设备响应）
void MainWindow::readUdpDatagram()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress senderAddr;
        quint16 senderPort;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddr, &senderPort);

        if (datagram == "LAN_FILE_TRANSFER_DISCOVER") {
            // 响应设备发现请求
            m_udpSocket->writeDatagram("LAN_FILE_TRANSFER_RESPONSE", senderAddr, 9999);
        } else if (datagram == "LAN_FILE_TRANSFER_RESPONSE") {
            // 收到其他设备响应，添加到列表
            QString deviceIp = senderAddr.toString();
            bool exists = false;
            for (int i = 0; i < ui->listWidget_devices->count(); ++i) {
                if (ui->listWidget_devices->item(i)->text().contains(deviceIp)) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                ui->listWidget_devices->addItem(new QListWidgetItem(QString("设备 (%1)").arg(deviceIp), ui->listWidget_devices));
            }
        }
    }
}

// 选择设备（核心修改：QRegExp → QRegularExpression）
void MainWindow::on_listWidget_devices_itemClicked(QListWidgetItem *item)
{
    QString text = item->text();
    // 替换QRegExp为QRegularExpression
    QRegularExpression ipRegex("\\d+\\.\\d+\\.\\d+\\.\\d+");
    // 替换indexIn为match，cap(0)为captured(0)
    QRegularExpressionMatch match = ipRegex.match(text);
    if (match.hasMatch()) {
        m_selectedDeviceIp = match.captured(0);
        ui->pushButton_sendFile->setEnabled(true);
    }
}

// 选择文件
void MainWindow::on_pushButton_selectFile_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择要发送的文件", QDir::homePath());
    if (!filePath.isEmpty()) {
        m_selectedFilePath = filePath;
        ui->label_selectedFile->setText(QFileInfo(filePath).fileName());
    }
}

// 发送文件
void MainWindow::on_pushButton_sendFile_clicked()
{
    if (m_selectedDeviceIp.isEmpty() || m_selectedFilePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择设备和文件！");
        return;
    }

    // 创建TCP客户端连接目标设备
    QTcpSocket *sendSocket = new QTcpSocket(this);
    sendSocket->connectToHost(QHostAddress(m_selectedDeviceIp), 8888);

    if (!sendSocket->waitForConnected(3000)) {
        QMessageBox::warning(this, "警告", "连接设备失败：" + sendSocket->errorString());
        sendSocket->deleteLater();
        return;
    }

    // 启动文件传输线程
    m_transferThread = new FileTransferThread(sendSocket, m_selectedFilePath, this);
    connect(m_transferThread, &FileTransferThread::progressChanged, this, &MainWindow::updateProgress);
    connect(m_transferThread, &FileTransferThread::transferFinished, this, &MainWindow::transferDone);
    connect(m_transferThread, &FileTransferThread::finished, m_transferThread, &QObject::deleteLater);
    connect(m_transferThread, &FileTransferThread::finished, sendSocket, &QObject::deleteLater);
    m_transferThread->start();

    ui->pushButton_sendFile->setEnabled(false);
    ui->progressBar->setValue(0);
}

// 更新传输进度
void MainWindow::updateProgress(int value)
{
    ui->progressBar->setValue(value);
}

// 传输完成
void MainWindow::transferDone(bool success)
{
    if (success) {
        QMessageBox::information(this, "提示", "文件发送成功！");
        // 将发送的文件添加到共享列表
        ui->listWidget_sharedFiles->addItem(QFileInfo(m_selectedFilePath).fileName());
    } else {
        QMessageBox::warning(this, "警告", "文件发送失败！");
    }
    ui->progressBar->setValue(0);
    ui->pushButton_sendFile->setEnabled(true);
}

// 新客户端连接（接收文件）
void MainWindow::newClientConnected()
{
    if (m_clientSocket) {
        m_clientSocket->abort();
        m_clientSocket->deleteLater();
    }

    m_clientSocket = m_tcpServer->nextPendingConnection();
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &MainWindow::readClientData);
    connect(m_clientSocket, &QTcpSocket::disconnected, m_clientSocket, &QObject::deleteLater);

    // 重置接收状态
    m_receivedSize = 0;
    m_fileSize = 0;
    if (m_receiveFile) {
        m_receiveFile->close();
        delete m_receiveFile;
        m_receiveFile = nullptr;
    }
}

// 读取客户端数据（接收文件）
void MainWindow::readClientData()
{
    QDataStream in(m_clientSocket);
    // 适配Qt 6：使用默认编译版本（移除Qt_5_15硬编码）
    in.setVersion(QDataStream::Qt_DefaultCompiledVersion);

    // 第一步：读取文件头（文件大小+文件名）
    if (m_fileSize == 0) {
        if (m_clientSocket->bytesAvailable() < (qint64)(sizeof(qint64) + sizeof(QString))) {
            return;
        }
        in >> m_fileSize >> m_selectedFilePath;

        // 创建接收文件
        QString savePath = QDir::homePath() + "/" + m_selectedFilePath;
        m_receiveFile = new QFile(savePath);
        if (!m_receiveFile->open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, "警告", "无法创建接收文件：" + m_receiveFile->errorString());
            m_clientSocket->abort();
            return;
        }
    }

    // 第二步：读取文件内容
    if (m_receivedSize < m_fileSize) {
        QByteArray buffer = m_clientSocket->readAll();
        qint64 writeSize = m_receiveFile->write(buffer);
        m_receivedSize += writeSize;

        ui->progressBar->setValue((int)((m_receivedSize * 100) / m_fileSize));

        // 接收完成
        if (m_receivedSize == m_fileSize) {
            m_receiveFile->close();
            QMessageBox::information(this, "提示", "文件接收成功！\n保存路径：" + QDir::homePath() + "/" + m_selectedFilePath);
            // 添加到共享列表
            ui->listWidget_sharedFiles->addItem(m_selectedFilePath);

            // 重置状态
            m_fileSize = 0;
            m_receivedSize = 0;
            m_receiveFile->deleteLater();
            m_receiveFile = nullptr;
            ui->progressBar->setValue(0);
        }
    }
}
