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


    m_frames.init();
    m_decoder.setFrames(&m_frames);
    m_videoWidget = new QYUVOpenGLWidget();
    m_videoWidget->resize(220, 450);

    connect(&m_decoder, &decoder::onNewFrame, this, [this]() {
        //qDebug() << "decoder::onNewFrame";
        m_frames.lock();
        const AVFrame * frame = m_frames.consumeRenderedFrame();
        //渲染frame
        m_videoWidget->setFrameSize(QSize(frame->width, frame->height));
        m_videoWidget->updateTextures(frame->data[0], frame->data[1], frame->data[2], frame->linesize[0], frame->linesize[1], frame->linesize[2]);
        m_frames.unLock();
    });


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
    m_videoWidget->show();
}


void Widget::on_stopserver_clicked()
{
    m_server.stop();
}

