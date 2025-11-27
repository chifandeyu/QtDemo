
#include "FileBufferManager.h"
#include "DataProducerThread.h"
#include "VirtualFileSrcStream.h"
#include <QDebug>
#include <QtGlobal>
#include <QWaitCondition>
#include <QCoreApplication>

namespace clipboard {

// 静态成员初始化
FileBufferManager* FileBufferManager::instance_ = nullptr;

FileBufferManager* FileBufferManager::instance()
{
    if (!instance_) {
        instance_ = new FileBufferManager();
    }
    return instance_;
}

void FileBufferManager::destroyInstance()
{
    if (instance_) {
        delete instance_;
        instance_ = nullptr;
    }
}

FileBufferManager::FileBufferManager(QObject *parent)
    : QObject(parent)
    , fileSize_(0)
    , totalBytesRead_(0)
    , transferActive_(false)
    , m_pVFSS(nullptr)
    , producerThread_(new DataProducerThread(this))
{
    m_transferComplete.store(false);
    // 连接信号和槽
//    connect(producerThread_, &DataProducerThread::dataChunkGenerated,
//            this, &FileBufferManager::onDataChunkGenerated, Qt::DirectConnection);
//    connect(producerThread_, &DataProducerThread::transferComplete,
//            this, &FileBufferManager::onTransferComplete);
    //connect(producerThread_, &QThread::finished, 
    //        this, &FileBufferManager::onProducerFinished);

    createVFS();
}

FileBufferManager::~FileBufferManager()
{
    stopTransfer();

    // 确保线程已停止
    if (producerThread_->isRunning()) {
        producerThread_->stop();
        producerThread_->wait(3000); // 最多等待3秒
    }
}

void FileBufferManager::startTransfer(const QString& filePath, const QString& fileName, qint64 fileSize)
{
    QMutexLocker locker(&m_mutex);

    // 如果已有传输在进行，先停止
    if (transferActive_) {
        stopTransfer();
    }

    // 重置状态
    fileName_ = fileName;
    fileSize_ = fileSize;
    totalBytesRead_ = 0;
    transferActive_ = true;
    m_transferComplete.store(false);

    // 清空队列
    dataQueue_.clear();

    // 配置并启动生产者线程
    producerThread_->setParameters(filePath, fileName, fileSize);
    producerThread_->start();
    if (!m_pVFSS) {
        createVFS();
    }
    else {
        m_pVFSS->setFileName(fileName_);
    }
    qDebug() << "file:" << fileName << "size:" << fileSize;
}

void FileBufferManager::stopTransfer()
{
    if (transferActive_) {
        transferActive_ = false;

        // 停止生产者线程
        producerThread_->stop();
        producerThread_->wait();

        qDebug() << "停止传输文件:" << fileName_;
    }
}

void FileBufferManager::createVFS()
{
    IDataObject *data_obj = nullptr;
    m_pVFSS = new VirtualFileSrcStream();
    m_pVFSS->setFileName(fileName_);
    HRESULT hr = m_pVFSS ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        m_pVFSS->onInit();
        hr = m_pVFSS->QueryInterface(IID_IDataObject, (void**)&data_obj);
        m_pVFSS->Release();
        if (SUCCEEDED(hr)) {
            ::OleSetClipboard(data_obj);
            data_obj->Release();
        }
    }
}

void FileBufferManager::clearVFS()
{
    m_pVFSS = nullptr;
}

qint64 FileBufferManager::readData(char *data, qint64 maxSize)
{
    // 如果传输未激活，返回0
    if (!transferActive_) {
        qDebug() << "stop transf read";
        return 0;
    }

    qDebug() << "------- readData maxSize: " << maxSize;

    // 等待数据队列非空或传输完成
    while (transferActive_ && !m_transferComplete.load()) {
        m_mutex.lock();
        if (dataQueue_.isEmpty()) {
            qDebug() << "dataQueue_ is empty, waiting for new data...";
            QCoreApplication::processEvents();
            QThread::msleep(20);
        }
        else {
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();
    }
    m_mutex.lock();
    // 如果传输完成且队列仍为空，返回0
    if (dataQueue_.isEmpty() && !transferActive_) {
        qDebug() << "dataQueue_.isEmpty() && !transferActive_";
        m_mutex.unlock();
        return 0;
    }
    qint64 bytesRead = 0;

    // 每次只从队列中取出一个数据块
    bool isEmpty = dataQueue_.isEmpty();
    if (!isEmpty) {
        QByteArray chunk = dataQueue_.dequeue();

        // 限制读取大小不超过请求的大小
        qint64 copySize = qMin((qint64)chunk.size(), maxSize);
        memcpy(data, chunk.constData(), copySize);
        bytesRead = copySize;

        // 如果块比需要的大，将剩余部分放回队列前端
        if (chunk.size() > copySize) {
            QByteArray remaining = chunk.mid(copySize);
            dataQueue_.prepend(remaining);
            qDebug() << "data to big, remaining.size: " << remaining.size() << " back to queue!";
        }

        qDebug() << "read buffer:" << copySize << ", dataQueue_ size: " << dataQueue_.size() << " block";
    } else {
        qDebug() << "dataQueue_.isEmpty read";
    }
    m_mutex.unlock();
    totalBytesRead_ += bytesRead;

    // 发送进度信号
    emit transferProgress(totalBytesRead_, fileSize_);

    qDebug() << "------- readData bytesRead: " << bytesRead << " totalBytesRead_:" << totalBytesRead_;
    // 如果已读取所有数据且传输已完成，发送完成信号
    if (totalBytesRead_ >= fileSize_ && !transferActive_) {
        qDebug() << "transferFinished read";
        emit transferFinished();
    }

    return bytesRead;
}

QString FileBufferManager::getFileName() const
{
    QMutexLocker locker(&m_mutex);
    return fileName_;
}

qint64 FileBufferManager::getFileSize() const
{
    QMutexLocker locker(&m_mutex);
    return fileSize_;
}

qint64 FileBufferManager::getTotalReadBytes() const
{
    QMutexLocker locker(&m_mutex);
    return totalBytesRead_;
}

bool FileBufferManager::isTransferComplete() const
{
    QMutexLocker locker(&m_mutex);
    return (!transferActive_ && totalBytesRead_ >= fileSize_);
}

void FileBufferManager::onDataChunkGenerated(const QByteArray& chunk)
{
    if (!transferActive_) {
        return;
    }

    // 限制队列大小，避免内存占用过高
    const int MAX_QUEUE_SIZE = 1000;
    m_mutex.lock();
    if (dataQueue_.size() >= MAX_QUEUE_SIZE) {
        qDebug() << "队列已满，当前大小：" << dataQueue_.size() << "，丢弃数据块";
        m_mutex.unlock();
        return;
    }

    // 添加到队列
    dataQueue_.enqueue(chunk);
//    qDebug() << "接收到数据块，大小：" << chunk.size() << "，队列大小：" << dataQueue_.size();
    m_mutex.unlock();
}

void FileBufferManager::onTransferComplete()
{
    qDebug() << "data producer data transferComplete!";

    // 不直接设置transferActive_为false，等待队列中的数据被消费完
    m_transferComplete.store(true);
}

void FileBufferManager::onProducerFinished()
{
    qDebug() << "data producer Finished";

    // 设置transferActive_为false，表示生产已完成
    transferActive_ = false;
}

} // namespace clipboard
