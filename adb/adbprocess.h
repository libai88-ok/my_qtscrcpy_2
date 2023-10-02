#ifndef ADBPROCESS_H
#define ADBPROCESS_H

#include <QProcess>

class AdbProcess : public QProcess
{
    Q_OBJECT
public:
    enum ADB_PROCESS_RESULT {
        AER_SUCCESS_START,      //启动成功
        AER_ERROR_START,        //启动失败
        AER_SUCCESS_EXEC,       //执行成功
        AER_ERROR_EXEC,         //执行失败
        AER_ERROR_MISSING_BINARY //找不到文件
    };

    AdbProcess(QObject *parent = nullptr);
    void execute(const QString & serial, const QStringList & args); //执行adb命令,封装了QProcess的start
    void push(const QString & serial, const QString & local, const QString &remote);    //执行adb push
    void removePath(const QString & serial, const QString & path);  //执行adb shell rm，删除作用
    void reverse(const QString & serial, const QString & deviceSocketName, quint16 localPort);  //建立adb reverse 反向连接
    void reverseRemove(const QString & serial, const QString & deviceSocketName);   //删除反向连接，adb reverse --remove
    QStringList getDeviceSerialFromStdOut();
    QString getDeviceIpFromStdOut();

    static QString getAdbPath();

signals:
    void adbProcessResult(ADB_PROCESS_RESULT processResult);

private:
    void initSignals();
    //void execute(const QString & serial, const QStringList & args);

    static QString s_adbPath;
    QString m_standardOutput = "";
    QString m_standardError = "";
};

#endif // ADBPROCESS_H
