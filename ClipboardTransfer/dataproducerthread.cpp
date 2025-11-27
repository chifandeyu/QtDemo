#include "DataProducerThread.h"
#include <QDebug>
#include <QThread>
#include "FileBufferManager.h"

namespace clipboard {

DataProducerThread::DataProducerThread(QObject* parent)
    : QThread(parent)
    , fileSize_(0)
    , totalBytesGenerated_(0)
    , shouldStop_(false)
    , file_(nullptr)
{
}

void DataProducerThread::setParameters(const QString& filePath, const QString& fileName, qint64 fileSize)
{
    filePath_ = filePath;
    fileName_ = fileName;
    fileSize_ = fileSize;
    totalBytesGenerated_ = 0;
    shouldStop_ = false;

    // 清理之前的文件对象
    if (file_) {
        delete file_;
        file_ = nullptr;
    }

    // 创建新的文件对象
    file_ = new QFile(filePath);
}

void DataProducerThread::stop()
{
    shouldStop_ = true;
}

DataProducerThread::~DataProducerThread()
{
    // 确保线程已停止
    if (isRunning()) {
        stop();
        wait(3000); // 最多等待3秒
    }

    // 清理文件对象
    if (file_) {
        delete file_;
        file_ = nullptr;
    }
}

void DataProducerThread::run()
{
    if (!file_) {
        qDebug() << "error: file is null!";
        return;
    }

    // 打开文件
    if (!file_->open(QIODevice::ReadOnly)) {
        qDebug() << "error: can't open file " << filePath_ << ":" << file_->errorString();
        return;
    }

    qDebug() << "DataProducerThread started, fileSize:" << fileSize_;

    while (!shouldStop_ && totalBytesGenerated_ < fileSize_) {
        // 计算剩余需要读取的字节数
        qint64 remainingBytes = fileSize_ - totalBytesGenerated_;

        // 确定本次读取的大小
        qint64 chunkSize = qMin((qint64)CHUNK_SIZE, remainingBytes);

        // 从文件读取数据
        QByteArray chunk = file_->read(chunkSize);

        // 检查是否成功读取了数据
        if (chunk.isEmpty()) {
            if (file_->atEnd()) {
                qDebug() << "file end, but size wrong!";
                break;
            } else {
                qDebug() << "read file failed:" << file_->errorString();
                break;
            }
        }

        // 更新计数器
        totalBytesGenerated_ += chunk.size();

        // 发送数据块
        FileBufferManager::instance()->onDataChunkGenerated(chunk);

        qDebug() << "read file block: " << chunk.size() << ", total:" << totalBytesGenerated_;

        // 短暂休眠，避免CPU占用过高
        msleep(SLEEP_INTERVAL);
    }

    // 关闭文件
    file_->close();

    if (totalBytesGenerated_ >= fileSize_) {
        qDebug() << "DataProducerThread finished, total read:" << totalBytesGenerated_;
        FileBufferManager::instance()->onTransferComplete();
    } else {
        qDebug() << "DataProducerThread stopped early, total read:" << totalBytesGenerated_;
    }
}

} // namespace clipboard
