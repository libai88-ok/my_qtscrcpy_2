#include "adbprocess.h"

#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
#include <Qt>
#include <QRegularExpression>
//#include <QProcess>


QString AdbProcess::s_adbPath = "";

QString AdbProcess::getAdbPath()
{
    if (s_adbPath.isEmpty()) {
        char* buf = nullptr;
        size_t sz = 0;
        _dupenv_s(&buf, &sz, "QTSCRCPY_ADB_PATH");
        s_adbPath = QString::fromLocal8Bit(buf);
        QFileInfo fileInfo(s_adbPath);
        //
        /*
         * qDebug() << QDir::currentPath(); 当前工作目录
         * qDebug() << QCoreApplication::applicationDirPath(); 程序所在目录
        */
        if (s_adbPath.isEmpty() || !fileInfo.isFile()) //如果通过环境变量指定的adbPath不存在，那么就去当前程序所在目录
        {
            s_adbPath = QCoreApplication::applicationDirPath() + "\\adb.exe";
        }
    }
    return s_adbPath;
}

AdbProcess::AdbProcess(QObject *parent)
    : QProcess(parent)
{
    initSignals();
    getAdbPath();
}

void AdbProcess::initSignals()
{
    connect(this, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
        if (error == ProcessError::FailedToStart) {
            emit adbProcessResult(AER_ERROR_MISSING_BINARY);
        }
        else {
            emit adbProcessResult(AER_ERROR_START);
        }
        qDebug() << "errorOccurred" << error;
    });
    connect(this, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
         [=](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitStatus == ExitStatus::NormalExit && exitCode == 0) {
            emit adbProcessResult(AER_SUCCESS_EXEC);
        }
        else {
            emit adbProcessResult(AER_ERROR_EXEC);
        }
        qDebug() << "finished" << exitCode << exitStatus;
    });

    //void errorOccurred(QProcess::ProcessError error)
    //void finished(int exitCode, QProcess::ExitStatus exitStatus = NormalExit)
    connect(this, &QProcess::readyReadStandardError, this, [this](){
        m_standardError = QString::fromLocal8Bit(readAllStandardError()).trimmed();
        qDebug() << "readyReadStandardError" << m_standardError;
    });

    connect(this, &QProcess::readyReadStandardOutput, this, [this](){
        m_standardOutput = QString::fromLocal8Bit(readAllStandardOutput()).trimmed();
        qDebug() << "readyReadStandardOutput" << m_standardOutput;
    });

    connect(this, &QProcess::started, this, [this](){
        emit adbProcessResult(AER_SUCCESS_START);
        qDebug() << "started";
    });
    /*connect(this, &QProcess::stateChanged, this, [this](QProcess::ProcessState state){
        qDebug() << "stateChanged" << state;
    });*

    /*void readyReadStandardError()
        void readyReadStandardOutput()
        void started()
        void stateChanged(QProcess::ProcessState newState)*/
}

void AdbProcess:: execute(const QString &serial, const QStringList &args)
{
    QStringList adbArgs;
    if (!serial.isEmpty()) {
        adbArgs << "-s" << serial;
    }
    adbArgs << args;
    qDebug() << adbArgs.join(" ");
    start(getAdbPath(), adbArgs);
}

void AdbProcess::push(const QString &serial, const QString &local, const QString &remote)
{
    QStringList adbArgs;
    adbArgs << "push";
    adbArgs << local;
    adbArgs << remote;
    //qDebug() << adbArgs.join(" ");
    execute(serial, adbArgs);
}

void AdbProcess::removePath(const QString &serial, const QString &path)
{
    QStringList adbArgs;
    adbArgs << "shell";
    adbArgs << "rm";
    adbArgs << path ;
    execute(serial, adbArgs);
}

void AdbProcess::reverse(const QString &serial, const QString &deviceSocketName, quint16 localPort)
{
    QStringList adbArgs;
    adbArgs << "reverse";
    adbArgs << QString("localabstract:%1").arg(deviceSocketName);
    adbArgs << QString("tcp:%1").arg(localPort);
    execute(serial, adbArgs);
}

void AdbProcess::reverseRemove(const QString &serial, const QString &deviceSocketName)
{
    QStringList adbArgs;
    adbArgs << "reverse";
    adbArgs << "--remove";
    adbArgs << QString("localabstract:%1").arg(deviceSocketName);
    execute(serial, adbArgs);
}

QStringList AdbProcess::getDeviceSerialFromStdOut()
{
    //QStringList adbArgs;
    //adbArgs << "devices";

    QStringList serials;
    QStringList devicesInfoList = m_standardOutput.split(QRegularExpression("\r\n|\n"), QString::SkipEmptyParts);
    for (QString & deviceInfo : devicesInfoList) {
        QStringList deviceInfos = deviceInfo.split(QRegularExpression("\t"), QString::SkipEmptyParts);
        if (deviceInfos.length() == 2 && !deviceInfos[1].compare("device")) {
            serials << deviceInfos[0];
        }
    }
    return serials;
}

QString AdbProcess::getDeviceIpFromStdOut()
{
    QString ip = "";
    //adb shell ip -f inet addr show/delete wlan0
    static QRegularExpression ipRegExp("inet [0-9.]*");
    QRegularExpressionMatch ipmatch = ipRegExp.match(m_standardOutput);
    if (ipmatch.hasMatch()) {
        ip = ipmatch.captured(0);
        ip = ip.right(ip.size() - 5);
    }
    return ip;
}

