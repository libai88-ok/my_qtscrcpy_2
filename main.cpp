#include "widget.h"
#include "decoder.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QTSCRCPY_ADB_PATH", "..\\qtscrcpy\\third_party\\adb\\win\\adb.exe"); //添加环境变量，以动态的方式获取adb.exe的路径
    qputenv("QTSCRCPY_SERVER_PATH", "..\\qtscrcpy\\third_party\\scrcpy-server.jar");

    //decoder::init();

    qDebug() << decoder::init();

    QApplication a(argc, argv);
    Widget w;
    w.show();
    int ret = a.exec();

    //qDebug() << decoder::deInit();
    return ret;

}
