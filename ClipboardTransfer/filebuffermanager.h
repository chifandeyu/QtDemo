
#pragma once
#include "dataproducerthread.h"

#include <QObject>
#include <QQueue>
#include <QByteArray>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QMutexLocker>

namespace clipboard {

class VirtualFileSrcStream;
class FileBufferManager : public QObject
{
    Q_OBJECT

public:
    // 单例模式
    static FileBufferManager* instance();
    static void destroyInstance();

    ~FileBufferManager();

    // 开始模拟网络传输，定时向队列中添加数据块
    void startTransfer(const QString& filePath, const QString& fileName, qint64 fileSize);

    // 停止传输
    void stopTransfer();

    bool isCurrentFile(int fileIndex) const {
        return fileIndex == next_expected_file_;
    }
    void markFileCompleted(int fileIndex) {
        if (fileIndex < file_transfer_completed_.size()) {
            file_transfer_completed_[fileIndex] = true;
            if (fileIndex == next_expected_file_) {
                // 移动到下一个文件
                next_expected_file_++;
            }
        }
    }
    int getNextExpectedFile() const {
        return next_expected_file_;
    }
    void createVFS();
    void clearVFS();

    // 从队列中读取数据，供FileStream::Read使用
    qint64 readData(char *data, qint64 maxSize);

    // 获取文件信息
    QString getFileName() const;
    qint64 getFileSize() const;
    qint64 getTotalReadBytes() const;

    // 检查是否传输完成
    bool isTransferComplete() const;

signals:
    void transferProgress(qint64 bytesTransferred, qint64 totalBytes);
    void transferFinished();

public slots:
    void onDataChunkGenerated(const QByteArray& chunk);
    void onTransferComplete();
    void onProducerFinished();

protected:
    FileBufferManager(QObject *parent = nullptr);

private:
    QQueue<QByteArray> dataQueue_;
    QString fileName_;
    qint64 fileSize_;
    qint64 totalBytesRead_;
    std::atomic<bool> transferActive_;
    std::atomic<bool> m_transferComplete;

    std::vector<bool> file_transfer_completed_;
    int next_expected_file_;

    DataProducerThread* producerThread_;
    VirtualFileSrcStream* m_pVFSS;
    mutable QMutex m_mutex;
    // 单例实例
    static FileBufferManager* instance_;
};

} // namespace clipboard
