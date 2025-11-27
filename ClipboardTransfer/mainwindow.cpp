
#include "mainwindow.h"
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QCoreApplication>

namespace clipboard {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , centralWidget_(nullptr)
    , transferInProgress_(false)
{
    // 初始化COM
    ::OleInitialize(nullptr);

    // 连接信号
    connect(FileBufferManager::instance(), &FileBufferManager::transferProgress, 
            this, &MainWindow::onTransferProgress);
    connect(FileBufferManager::instance(), &FileBufferManager::transferFinished, 
            this, &MainWindow::onTransferFinished);

    // 设置UI
    setupUI();

    // 设置窗口标题和大小
    setWindowTitle(tr("clip trans demo"));
    resize(600, 400);

    QString fileName = "D:\\005799\\Downloads\\Notepad--v3.6.0-plugin-Installer.exe";
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        filePathEdit_->setText(fileInfo.fileName());

        // 更新文件大小显示
        qint64 fileSizeBytes = fileInfo.size();
        QString sizeStr = formatFileSize(fileSizeBytes);
        fileSizeDisplayLabel_->setText(tr("file size: %1").arg(sizeStr));

        // 存储完整路径以备后用
        selectedFilePath_ = fileName;
    }
}

MainWindow::~MainWindow()
{
    // 清理COM
    OleUninitialize();
    // 销毁单例
    FileBufferManager::destroyInstance();
}

void MainWindow::setupUI()
{
    // 创建中央部件
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);

    // 创建主布局
    mainLayout_ = new QVBoxLayout(centralWidget_);

    // 创建文件选择区域
    fileGroup_ = new QGroupBox("file info", this);
    fileLayout_ = new QHBoxLayout(fileGroup_);

    filePathEdit_ = new QLineEdit(fileGroup_);
    filePathEdit_->setPlaceholderText("select file");
    filePathEdit_->setText("test.txt");

    browseButton_ = new QPushButton(tr("view..."), fileGroup_);
    connect(browseButton_, &QPushButton::clicked, this, &MainWindow::onSelectFile);

    fileLayout_->addWidget(filePathEdit_);
    fileLayout_->addWidget(browseButton_);

    // 创建设置区域
    settingsGroup_ = new QGroupBox(tr("file info"), this);
    settingsLayout_ = new QVBoxLayout(settingsGroup_);

    // 添加文件大小显示标签
    fileSizeDisplayLabel_ = new QLabel(tr("file size: --"), settingsGroup_);
    settingsLayout_->addWidget(fileSizeDisplayLabel_);

    // 创建控制按钮区域
    buttonLayout_ = new QHBoxLayout();

    startButton_ = new QPushButton(tr("start transf"), this);
    startButton_->setEnabled(true);
    connect(startButton_, &QPushButton::clicked, this, &MainWindow::onStartTransfer);

    cancelButton_ = new QPushButton(tr("cancel transf"), this);
    cancelButton_->setEnabled(false);
    connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::onCancelTransfer);

    buttonLayout_->addWidget(startButton_);
    buttonLayout_->addWidget(cancelButton_);
    buttonLayout_->addStretch();

    // 创建进度显示区域
    progressGroup_ = new QGroupBox(tr("progress"), this);
    progressLayout_ = new QVBoxLayout(progressGroup_);

    progressBar_ = new QProgressBar(progressGroup_);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    statusLabel_ = new QLabel(tr("aleady..."), progressGroup_);

    progressLayout_->addWidget(progressBar_);
    progressLayout_->addWidget(statusLabel_);

    // 添加所有区域到主布局
    mainLayout_->addWidget(fileGroup_);
    mainLayout_->addWidget(settingsGroup_);
    mainLayout_->addLayout(buttonLayout_);
    mainLayout_->addWidget(progressGroup_);
    mainLayout_->addStretch();
}

void MainWindow::onStartTransfer()
{
    // 获取文件名和大小
    QString fileName = filePathEdit_->text();
    if (fileName.isEmpty() || selectedFilePath_.isEmpty()) {
        QMessageBox::warning(this, "error", "please select file");
        return;
    }

    // 使用实际文件大小
    QFileInfo fileInfo(selectedFilePath_);
    qint64 fileSizeBytes = fileInfo.size();
    qDebug() << "使用实际文件大小:" << fileSizeBytes << "字节";

    // 更新UI状态
    transferInProgress_ = true;
    startButton_->setEnabled(false);
    cancelButton_->setEnabled(true);
    filePathEdit_->setEnabled(false);

    browseButton_->setEnabled(false);

    // 重置进度
    progressBar_->setValue(0);
    statusLabel_->setText(tr("eleady transf..."));

    // 开始传输
    FileBufferManager::instance()->startTransfer(selectedFilePath_, fileName, fileSizeBytes);
}

void MainWindow::onCancelTransfer()
{
    if (transferInProgress_) {
        FileBufferManager::instance()->stopTransfer();
        resetUI();
        statusLabel_->setText(tr("has candel!"));
    }
}

void MainWindow::onTransferProgress(qint64 bytesTransferred, qint64 totalBytes)
{
    // 计算进度百分比
    int progress = totalBytes > 0 ? static_cast<int>((bytesTransferred * 100) / totalBytes) : 0;
    progressBar_->setValue(progress);

    // 更新状态文本
    QString transferredStr = formatFileSize(bytesTransferred);
    QString totalStr = formatFileSize(totalBytes);
    statusLabel_->setText(QString("transf: %1 / %2 (%3%)")
                         .arg(transferredStr)
                         .arg(totalStr)
                         .arg(progress));
}

void MainWindow::onTransferFinished()
{
    resetUI();
    statusLabel_->setText("transf finished");
}

void MainWindow::onSelectFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "select file", "", "all (*.*)");
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        filePathEdit_->setText(fileInfo.fileName());

        // 更新文件大小显示
        qint64 fileSizeBytes = fileInfo.size();
        QString sizeStr = formatFileSize(fileSizeBytes);
        fileSizeDisplayLabel_->setText(tr("file size: %1").arg(sizeStr));

        // 存储完整路径以备后用
        selectedFilePath_ = fileName;
    }
}

void MainWindow::resetUI()
{
    transferInProgress_ = false;
    startButton_->setEnabled(true);
    cancelButton_->setEnabled(false);
    filePathEdit_->setEnabled(true);
    browseButton_->setEnabled(true);
}

QString MainWindow::formatFileSize(qint64 bytes) const
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (bytes >= GB) {
        return QString("%1 GB").arg(static_cast<double>(bytes) / GB, 0, 'f', 2);
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(static_cast<double>(bytes) / MB, 0, 'f', 2);
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(static_cast<double>(bytes) / KB, 0, 'f', 2);
    } else {
        return QString("%1 B").arg(bytes);
    }
}

}
