#include "devicesocket.h"
#include "common/qscrcpyevent.h"

#include <QtGlobal>
#include <QCoreApplication>
#include <QThread>
#include <QMutexLocker>

DeviceSocket::DeviceSocket(QObject *parent)
    :QTcpSocket(parent)
{
    connect(this, &DeviceSocket::readyRead, this, &DeviceSocket::onReadyRead);
    connect(this, &DeviceSocket::disconnected, this, &DeviceSocket::quitNotify);
    connect(this, &DeviceSocket::aboutToClose, this, &DeviceSocket::quitNotify);

}

DeviceSocket::~DeviceSocket()
{

}

//消费者
qint32 DeviceSocket::subThreadRecvData(quint8 *buf, qint32 bufSize)
{
    //保证函数在子进程中调用
    Q_ASSERT(QCoreApplication::instance()->thread() != QThread::currentThread());
    if (m_quit) {
        return 0;
    }

    QMutexLocker locker(&m_mutex);

    m_buf = buf;
    m_bufferSize = bufSize;

    //可能onReadyRead已经读取数据发送wake，才调用subThreadRecvData，这样waitcondition阻塞，直到下一个readyRead信号调用onReadyRead；造成数据丢失
    //在等待之前发送事件，主动调用onReadRead；使用QT的事件
    DeviceSocketEvent * getDataEvent = new DeviceSocketEvent();
    //这个event可以不用手动释放，在event函数return后会自动释放
    QCoreApplication::postEvent(this, getDataEvent);


    while (!m_recvData) {
        //在wait执行期间，mutex一直上锁，直到调用os的wait_block阻塞瞬间将mutex解锁；当被另一进程唤醒后，重新将mutex上锁
        //确保wake一定在wait后执行
        m_recvDataCond.wait(&m_mutex);
    }

    m_recvData = false;
    return m_dataSize;
}

bool DeviceSocket::event(QEvent * event)
{
    if (event->type() == qscrcpyevent::DeviceSocket) {
        onReadyRead();
        return true;
    }
    return QTcpSocket::event(event);
}

void DeviceSocket::onReadyRead()
{
    QMutexLocker locker(&m_mutex);
    if (m_buf && bytesAvailable() > 0) {
        //接受数据
        qint64 readSize = qMin(bytesAvailable(), (qint64)m_bufferSize);
        m_dataSize = read((char *)m_buf, readSize);

        m_buf = Q_NULLPTR;
        m_bufferSize = 0;
        m_recvData = true;
        m_recvDataCond.wakeOne();
    }

}

void DeviceSocket::quitNotify()
{
    m_quit = true;
    QMutexLocker locker(&m_mutex);
    if (m_buf) {
        m_buf = Q_NULLPTR;
        m_bufferSize = 0;
        m_recvData = true;
        m_recvDataCond.wakeOne();
    }

}

