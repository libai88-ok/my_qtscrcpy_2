#ifndef DEVICESOCKET_H
#define DEVICESOCKET_H

#include <QTcpSocket>
//条件变量
#include <QWaitCondition>
//互斥锁
#include <QMutex>

class DeviceSocket : public QTcpSocket
{
    Q_OBJECT
public:
    DeviceSocket(QObject *parent = nullptr);
    ~DeviceSocket();

    qint32 subThreadRecvData(quint8 * buf, qint32 bufSize);

protected:
    //实现event中一个虚函数
    bool event(QEvent * event);

protected slots:
    void onReadyRead();
    void quitNotify();

private:
    QMutex m_mutex;
    QWaitCondition m_recvDataCond;

    //标志
    bool m_recvData = false;
    bool m_quit = false;

    //buf缓存用来接受收到的数据
    quint8 * m_buf= Q_NULLPTR;
    qint32 m_bufferSize = 0;
    qint32 m_dataSize = 0;


};

#endif // DEVICESOCKET_H
