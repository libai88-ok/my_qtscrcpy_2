#include "server.h" 

#include <QCoreApplication>
#include <QFileInfo>
//#include <QtDebug>
#include <QSize>

#define DEVICE_SERVER_PATH "/data/local/tmp/scrcpy-server.jar"
#define SOCKET_NAME "scrcpy"
#define DEVICE_NAME_FIELD_LENGTH 64

server::server(QObject *parent)
    :QObject(parent)
{
    connect(&m_workProcess, &AdbProcess::adbProcessResult, this, &server::onWorkProcessResult);
    connect(&m_serverProcess, &AdbProcess::adbProcessResult, this, &server::onWorkProcessResult);
    connect(&m_serverSocket, &QTcpServer::newConnection, this,[this](){
        m_deviceSocket = dynamic_cast<DeviceSocket *>(m_serverSocket.nextPendingConnection());

        QString deviceName;
        QSize size;
        //socket接收第一条信息包含device name与size
        if (m_deviceSocket && m_deviceSocket->isValid() && readInfo(deviceName, size)) {
            //socket建立成功那么就可以关闭反向代理
            disableTunnelReverse();
            //安卓会对jar加标记，等到jar执行结束再删除
            removeServer();
            emit connectToResult(true, deviceName, size);
        } else {
            stop();
            emit connectToResult(false, deviceName, size);
        }
    });
}

/*
 * 1.将scrcpy push到手机上
* 2.反向连接端口
* 3.启动服务
* （如果失败都需请理前面的步骤）
*/

bool server::start(const QString &serial, quint16 localPort, quint16 maxSize, quint16 bitRate)
{
    m_serial = serial;
    m_localPort = localPort;
    m_maxSize = maxSize;
    m_bitRate = bitRate;

    m_serverStartStep = SSS_PUSH;

    return startServerByStep();
}

void server::stop()
{
    if (m_deviceSocket) {
        m_deviceSocket->close();
        //不需要手动删除，因为这个指针是调用parent中函数获取的，它的析构由parent负责
        //m_deviceSocket->deleteLater();
    }
    m_serverProcess.kill();
    disableTunnelReverse();
    removeServer();
    m_serverSocket.close();
}

//根据adbprocess中信号进行跳转
void server::onWorkProcessResult(AdbProcess::ADB_PROCESS_RESULT processResult)
{
    if (sender() == &m_workProcess) {
        if (m_serverStartStep != SSS_NULL) {
            switch (m_serverStartStep) {
            case SSS_PUSH:
                if (processResult == AdbProcess::AER_SUCCESS_EXEC) {
                    m_serverStartStep = SSS_ENABLE_REVERSE;
                    m_serverCopiedToDevice = true;
                    startServerByStep();
                } else if (processResult == AdbProcess::AER_ERROR_EXEC || processResult == AdbProcess::AER_ERROR_MISSING_BINARY ||processResult == AdbProcess::AER_ERROR_START){
                    qCritical() << "adb push failed!";
                    m_serverStartStep = SSS_NULL;
                    emit serverStartResult(false);
                }
                break;
            case SSS_ENABLE_REVERSE:
                if (processResult == AdbProcess::AER_SUCCESS_EXEC) {
                    m_serverStartStep = SSS_EXECUTE_SERVER;
                    m_enableReverse = true;
                    startServerByStep();
                } else if(processResult == AdbProcess::AER_ERROR_EXEC || processResult == AdbProcess::AER_ERROR_MISSING_BINARY ||processResult == AdbProcess::AER_ERROR_START){
                    qCritical() << "adb reverse failed!";
                    m_serverStartStep = SSS_NULL;
                    //这里失败处理不同，因为上一步失败只需报错，这一步失败需要将上一步push的删除
                    removeServer();
                    emit serverStartResult(false);
                }
                break;
            case SSS_EXECUTE_SERVER:

                break;
            default:
                break;
            }
        }
    }

    if (sender() == &m_serverProcess) {
        if (SSS_EXECUTE_SERVER == m_serverStartStep) {
            //因为这里的adb shell是阻塞的，所以只判断是否启动成功，而不是成功执行
            if (processResult == AdbProcess::AER_SUCCESS_START) {
                m_serverStartStep = SSS_RUNNING;
                emit serverStartResult(true);
            }
            else if (processResult == AdbProcess::AER_ERROR_START){
                qCritical() << "adb shell start server failed!";
                m_serverStartStep = SSS_NULL;
                disableTunnelReverse();
                removeServer();
                emit serverStartResult(false);
            }
        }
    }

}

//根据状态变量，设置一个状态机，然后一步步执行
bool server::startServerByStep()
{
    bool stepSuccess = false;

    if(m_serverStartStep != SSS_NULL) {
        switch (m_serverStartStep) {
        case SSS_PUSH:
            //使用封装好的adb进程，初始化一个adbprocess
            stepSuccess = pushServer();
            break;
        case SSS_ENABLE_REVERSE:
            stepSuccess = enableTunnelReverse();
            break;
        case SSS_EXECUTE_SERVER:
            //PC端监听本地端口
            m_serverSocket.setMaxPendingConnections(1);
            if (!m_serverSocket.listen(QHostAddress::LocalHost, m_localPort)) {
                qCritical() << QString("Could not listen on port %1").arg(m_localPort);
                m_serverStartStep = SSS_NULL;
                disableTunnelReverse();
                removeServer();
                emit serverStartResult(false);
                return false;
            }
            stepSuccess = execute();
            break;
        case SSS_RUNNING:

            break;
        default:
            break;
        }
    }
    return stepSuccess;
}

bool server::pushServer()
{
    m_workProcess.push(m_serial, getServerPath(), DEVICE_SERVER_PATH);
    return true;
}

//这里不关心remove的处理结果，所以最好不适用m_workProcess处理流程
bool server::removeServer()
{
    if (!m_serverCopiedToDevice) {
        return true;
    }
    m_serverCopiedToDevice = false;
    AdbProcess * thisProcess = new AdbProcess();
    if (!thisProcess) {
        return false;
    }
    connect(thisProcess, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_PROCESS_RESULT processResult){
        if (processResult != AdbProcess::AER_SUCCESS_START) {
            //sender() 返回发送信号的对象的指针，调用deleteLater，
            sender()->deleteLater();
        }
    });
    thisProcess->removePath(m_serial, DEVICE_SERVER_PATH);
    return true;
}

bool server::enableTunnelReverse()
{
    m_workProcess.reverse(m_serial, SOCKET_NAME, m_localPort);
    return true;
}

//adb shell CLASSPATH=/data/local/tmp/scrcpy-server.jar app_process / com.genymobile.scrcpy.Server maxsize bitrate false ""
//adb shell命令 启动这个jar文件中 斜杠后jar中这个类的main函数，启动scrcpy-server；
//后接参数，最大尺寸:0是默认，1080better、比特率8000000、是否正向连接（反向false）、是否对画面进行剪切（不剪切设为空）
bool server::execute()
{
    QStringList args;
    args << "shell";
    args << QString("CLASSPATH=%1").arg(DEVICE_SERVER_PATH);
    args << "app_process";
    args << "/";
    args << "com.genymobile.scrcpy.Server";
    args << QString::number(m_maxSize);
    args << QString::number(m_bitRate);
    args << "false";
    args << "";
    m_serverProcess.execute(m_serial,args);
    return true;
}

bool server::disableTunnelReverse()
{
    if (!m_enableReverse) {
        return true;
    }
    m_enableReverse = false;
    AdbProcess * thisProcess = new AdbProcess();
    if (!thisProcess) {
        return false;
    }
    connect(thisProcess, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_PROCESS_RESULT processResult){
        if (processResult != AdbProcess::AER_SUCCESS_START) {
            //sender() 返回发送信号的对象的指针，调用deleteLater，
            sender()->deleteLater();
        }
    });
    thisProcess->reverseRemove(m_serial, SOCKET_NAME);
    return true;
}

DeviceSocket *server::getDeviceSocket()
{
    return m_deviceSocket;
}

QString server::getServerPath()
{
    if (m_serverPath.isEmpty()) {
        char* buf = nullptr;
        size_t sz = 0;
        _dupenv_s(&buf, &sz, "QTSCRCPY_SERVER_PATH");
        m_serverPath = QString::fromLocal8Bit(buf);
        QFileInfo fileInfo(m_serverPath);
        //
        /*
         * qDebug() << QDir::currentPath(); 当前工作目录
         * qDebug() << QCoreApplication::applicationDirPath(); 程序所在目录
        */
        if (m_serverPath.isEmpty() || !fileInfo.isFile()) //如果通过环境变量指定的adbPath不存在，那么就去当前程序所在目录
        {
           m_serverPath = QCoreApplication::applicationDirPath() + "\\scrcpy-server.jar";
        }
    }
    return m_serverPath;
}

bool server::readInfo(QString &deviceName, QSize &size)
{
    //64bit 设备名称 2b 宽 2b高
    unsigned char buf[DEVICE_NAME_FIELD_LENGTH + 4];
    //如果socket中b小于64那么等待300毫秒
    if (m_deviceSocket->bytesAvailable() <= DEVICE_NAME_FIELD_LENGTH + 4) {
        m_deviceSocket->waitForReadyRead(300);
        //return false;
    }
    qint64 len = m_deviceSocket->read((char *)buf, sizeof(buf));
    if (len < DEVICE_NAME_FIELD_LENGTH + 4) {
        qInfo() << "Could not retrieve device infomation";
        return false;
    }
    buf[DEVICE_NAME_FIELD_LENGTH-1] = '\0';
    deviceName = (char *) buf;
    size.setWidth((buf[DEVICE_NAME_FIELD_LENGTH] << 8) | buf[DEVICE_NAME_FIELD_LENGTH+1]);
    size.setHeight((buf[DEVICE_NAME_FIELD_LENGTH+2] << 8) | buf[DEVICE_NAME_FIELD_LENGTH+3]);
    return true;
}
