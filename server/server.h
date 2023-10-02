#ifndef SERVER_H
#define SERVER_H

#include <QObject>

#include "../adb/adbprocess.h"
#include "devicesocket.h"
#include "tcpserver.h"

class server : public QObject
{
    Q_OBJECT
public:
    enum SERVER_START_STEP{
        SSS_NULL,
        SSS_PUSH,
        SSS_ENABLE_REVERSE,
        SSS_EXECUTE_SERVER,
        SSS_RUNNING
    };

public:
    server(QObject *parent = nullptr);

    bool start(const QString & serial, quint16 localPort, quint16 maxSize, quint16 bitRate);
    void stop();
     DeviceSocket * getDeviceSocket();
signals:
    void serverStartResult(bool success);
    void connectToResult(bool success, const QString & deviceName, const QSize & size);
private slots:
    void onWorkProcessResult(AdbProcess::ADB_PROCESS_RESULT processResult);

private:
    bool startServerByStep();
    bool pushServer();
    bool removeServer();
    bool enableTunnelReverse();
    bool execute();
    bool disableTunnelReverse();

    QString getServerPath();

private:
    QString m_serial = "";
    quint16 m_localPort = 0;
    quint16 m_maxSize = 0;
    quint16 m_bitRate = 0;

    SERVER_START_STEP m_serverStartStep = SSS_NULL;

    AdbProcess m_workProcess;
    AdbProcess m_serverProcess;

    QString m_serverPath;

    bool m_serverCopiedToDevice = false;
    bool m_enableReverse = false;

    //QTcpSocket是线程敏感的，并且解码视频需要时间
    TcpServer m_serverSocket;

    bool readInfo(QString & deviceName, QSize & size);
    DeviceSocket * m_deviceSocket = nullptr;
};

#endif // SERVER_H
