#include "widget.h"
#include "ui_widget.h"


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    connect(&m_server, &server::serverStartResult, this, [this](bool success) {
            qDebug() << "server start" << success;
        });
    connect(&m_server, &server::connectToResult, this, [this](bool success, const QString & deviceName, const QSize & size) {
        qDebug() << "server connection" << success << "devicename:" << deviceName << "size_w:" <<size.width() << "size_h:" << size.height();
        if(success) {
            m_decoder.setDeviceSocket(m_server.getDeviceSocket());
            m_decoder.startDecode();
        }
    });
    connect(&m_decoder, &decoder::onNewFrame, this, [this]() {
        qDebug() << "decoder::onNewFrame";
    });
    m_frames.init();
    m_decoder.setFrames(&m_frames);

}

Widget::~Widget()
{
    m_frames.deInit();
    delete ui;
}


void Widget::on_startserver_clicked()
{
    //QObject *parent;
        //QStringList arguments;
        //arguments << "devices";

       /* arguments << "shell";
        arguments << "ip";
        arguments << "-f";
        arguments << "inet";
        arguments << "addr";
        arguments << "show";
        arguments << "wlan0";
        AdbProcess *myProcess = new AdbProcess(this);
        connect(myProcess, &AdbProcess::adbProcessResult, this, [this, myProcess](AdbProcess::ADB_PROCESS_RESULT processResult){
            qDebug() << ">>>>>>>>" << processResult;
            if (processResult == AdbProcess::AER_SUCCESS_EXEC) {
                qDebug() << myProcess->getDeviceIpFromStdOut();
            }
        });
        //QString program = myProcess->getAdbPath();
        //qDebug() << QDir::currentPath();;

        //myProcess->removePath("", "/sdcard/Download/test.txt");
        //myProcess->reverseRemove("", "scrcpy");
        myProcess->execute("", arguments);*/
    m_server.start("", 27183, 720, 8000000);
}


void Widget::on_stopserver_clicked()
{
    m_server.stop();
}

