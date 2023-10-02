#ifndef DECODER_H
#define DECODER_H

#include <QThread>
#include <QPointer>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}
class Frames;
class DeviceSocket;

class decoder : public QThread
{
    Q_OBJECT
public:
    decoder();
    static bool init();
    static void deInit();
    void setFrames(Frames* frames);
    void setDeviceSocket(DeviceSocket* deviceSocket);
    bool startDecode();
    void stopDecode();
    qint32 recvData(quint8* buf, qint32 bufSize);

signals:
    void onNewFrame();
    void onDecodeStop();

protected:
    void run();
    void pushFrame();

private:
    //接受h264数据
    QPointer<DeviceSocket> m_deviceSocket;
    //推出标记
    bool m_quit = false;
    //解码出的帧
    Frames * m_frames;
};

#endif // DECODER_H
