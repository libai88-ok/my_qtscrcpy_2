#ifndef FRAMES_H
#define FRAMES_H

#include <QMutex>
#include <QtGlobal>

typedef struct AVFrame AVFrame;

class Frames
{
public:
    Frames();
    virtual ~Frames();

    bool init();
    void deInit();

    void lock();
    void unLock();

    //获取解码帧
    AVFrame* decodingFrame();
    //交换解码帧与渲染帧
    bool offerDecodedFrame();
    //提供渲染器的接口
    const AVFrame* consumeRenderedFrame();
    void stop();

private:
    void swap();

private:
    //刚解码出的一帧（yuv）数据
    AVFrame * m_decodingFrame = Q_NULLPTR;
    //正在渲染的一帧yuv数据
    AVFrame * m_renderingframe = Q_NULLPTR;
    //多线程安全，解码线程vs渲染线程
    QMutex m_mutex;
    bool m_renderingFrameConsumed = true;

};

#endif // FRAMES_H
