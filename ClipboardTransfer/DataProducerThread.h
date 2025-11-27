#ifndef DATAPRODUCERTHREAD_H
#define DATAPRODUCERTHREAD_H

#include <QThread>
#include <QFile>

namespace clipboard {

// 工作线程类，用于生产数据
class DataProducerThread : public QThread
{
    Q_OBJECT
public:
    explicit DataProducerThread(QObject* parent = nullptr);
    ~DataProducerThread();
    void setParameters(const QString& filePath, const QString& fileName, qint64 fileSize);
    void stop();

protected:
    void run() override;

signals:
//    void dataChunkGenerated(const QByteArray& chunk);
//    void transferComplete();

private:
    QString filePath_;
    QString fileName_;
    qint64 fileSize_;
    qint64 totalBytesGenerated_;
    bool shouldStop_;
    QFile* file_;
    static const int CHUNK_SIZE = 512 * 1024; // 512KB chunks
    static const int SLEEP_INTERVAL = 20; // 10ms intervals
};
}
#endif // DATAPRODUCERTHREAD_H
