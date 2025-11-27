
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <Windows.h>
#include <shlobj.h>
#include "FileBufferManager.h"
#include "VirtualFileSrcStream.h"

namespace clipboard {

class VirtualFileSrcStream;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartTransfer();
    void onCancelTransfer();
    void onTransferProgress(qint64 bytesTransferred, qint64 totalBytes);
    void onTransferFinished();
    void onSelectFile();

private:
    void setupUI();
    void resetUI();
    QString formatFileSize(qint64 bytes) const;

    // UI组件
    QWidget *centralWidget_;
    QVBoxLayout *mainLayout_;

    // 文件选择区域
    QGroupBox *fileGroup_;
    QHBoxLayout *fileLayout_;
    QLineEdit *filePathEdit_;
    QPushButton *browseButton_;

    // 设置区域
    QGroupBox *settingsGroup_;
    QVBoxLayout *settingsLayout_;
    QLabel *fileSizeDisplayLabel_;

    // 控制按钮区域
    QHBoxLayout *buttonLayout_;
    QPushButton *startButton_;
    QPushButton *cancelButton_;

    // 进度显示区域
    QGroupBox *progressGroup_;
    QVBoxLayout *progressLayout_;
    QProgressBar *progressBar_;
    QLabel *statusLabel_;

    // 状态变量
    bool transferInProgress_;
    QString selectedFilePath_; // 存储选择的文件完整路径
};

}

#endif // MAINWINDOW_H
