#include "decoder.h"
#include "server/devicesocket.h"
#include "frames.h"

#define BUFFSIZE 0x100000

decoder::decoder()

{

}

bool decoder::init()
{
    //调用所有ffmpeg前，初始化
    if (avformat_network_init()) {
            return false;
    }
    return true;
}

void decoder::deInit()
{
    avformat_network_deinit();
}

void decoder::setFrames(Frames *frames)
{
    //保存frames帧-解码后
    m_frames = frames;
}

static qint32 readPacket(void *opaque, uint8_t *buf, qint32 buf_size) {
    decoder * cdecoder = (decoder*)opaque;
    if (cdecoder) {
        return cdecoder->recvData(buf, buf_size);
    }
    return 0;

}

void decoder::setDeviceSocket(DeviceSocket *deviceSocket)
{
    m_deviceSocket = deviceSocket;
}

bool decoder::startDecode()
{
    if (!m_deviceSocket) {
        return false;
    }
    m_quit = false;
    start();
    return true;
}

void decoder::stopDecode()
{
    m_quit = true;
    if (m_frames) {
        m_frames->stop();
    }
    //等待解码线程退出
    wait();
}

qint32 decoder::recvData(quint8 *buf, qint32 bufSize)
{
    if (!buf) {
        return 0;
    }
    if (m_deviceSocket) {
        qint32 len = m_deviceSocket->subThreadRecvData(buf, bufSize);
        qDebug() << "rec data" << len;
        if (len == -1) {
            return AVERROR(errno);
        }
        if (len == 0) {
            return AVERROR_EOF;
        }
        return len;
    }
    return AVERROR_EOF;
}

//重载QThread中的run，开始标准ffmpeg解码流程
void decoder::run()
{
    //代表保存解码过程中 临时数据 的缓冲区
    unsigned char * decodeBuffer = Q_NULLPTR;
    //用于读取数据=io上下文
    AVIOContext * avioCtx = Q_NULLPTR;
    //封装上下文，视频的封装格式；作用：解封装
    //虽然qtscrcpy直接从h264编码数据中读取，不需要从文件中解封装读取；但是这是ffmpeg的一个标准流程，通过一些设置直接从内存中读取h264数据
    AVFormatContext * formatCtx = Q_NULLPTR;
    //真正的解码器，解h264
    AVCodec * codec = Q_NULLPTR;
    //解码器上下文，与解码器组合使用
    AVCodecContext * codecCtx = Q_NULLPTR;
    //请理标记；run初始化的时候设置为false
    //封装上下文解码器的请理标志
    bool isFormatCtxOpen = false;
    //解码上下文解码器请理标志
    bool isCodecCtxOpen = false;

    //申请分配这个临时解码缓冲区
    decodeBuffer = (unsigned char *) av_malloc(BUFFSIZE);
    if (!decodeBuffer) {
        qCritical() << "can not allocate buffer";
        goto runQuit;
    }
    //初始化io解码上下文
    //参数： 1.解码缓冲区 2.bufsize 3.写标记 4. 后一个函数的参数 5.因为不从本地封装（mp4）读取，自定义一个从内存中读取的函数 6.写函数，设置为空 7. null
    //解码器运行回调readPacket函数，数据读取回调函数
    avioCtx = avio_alloc_context(decodeBuffer, BUFFSIZE, 0, this, readPacket, NULL, NULL);
    if (!avioCtx) {
        qCritical() << "can not allocate avio ctx";
        goto runQuit;
    }

    formatCtx = avformat_alloc_context();
    if (!formatCtx) {
        qCritical() << "can not allocate format ctx";
        goto runQuit;
    }

    formatCtx->pb = avioCtx;
    //打开封装上下文
    if (avformat_open_input(&formatCtx, NULL, NULL, NULL) < 0) {
        qCritical() << "can not open format ctx";
        goto runQuit;
    }
    isFormatCtxOpen = true;
    //初始化解码器
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        qCritical() << "H264 decoder not found";
        goto runQuit;
    }
    //初始化上下文解码器
    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        qCritical() << "can not allocate decoder ctx";
        goto runQuit;
    }
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        qCritical() << "can not open h264 codec";
        goto runQuit;
    }
    isCodecCtxOpen = true;

    //下面开始正式解码
    //一个avpacket代表一个视频解码数据包，保存解码前的一帧h264数据，通过readpacket函数读取数据，直到一帧才开始解码
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = Q_NULLPTR;
    packet.size = 0;

    //从封装上下文中读取一帧解码前数据,保存到avpacket中
    while (!m_quit && !av_read_frame(formatCtx, &packet)) {
        //保存解码后的数据帧,yuv
        AVFrame * decodingFrame = m_frames->decodingFrame();
        int ret;
        //将packet发送到codecCtx中，进行解码；函数执行完毕后，解码后数据已经保存在缓冲区中
        if ((ret = avcodec_send_packet(codecCtx, &packet)) < 0) {
            qCritical() << QString("can not send video packet: %1").arg(ret);
            goto runQuit;
        }
        //qDebug() << QString("avcodec_send_packet: %1").arg(ret);
        //将数据从缓冲区取出到decodingFrame
        if (decodingFrame) {
            ret = avcodec_receive_frame(codecCtx, decodingFrame);
        }
        //qDebug() << QString("avcodec_receive_frame: %1").arg(ret);
        if (!ret) {
            //qDebug() << "pushFrame";
            pushFrame();
        } else if (ret != AVERROR(EAGAIN)) { //缓冲区未存满
            qCritical() << QString("can not receive video frame: %1").arg(ret);
            goto runQuit;
        }

        //每次解完数据后清除之前步骤中添加的依赖
        av_packet_unref(&packet);

        if (avioCtx->eof_reached) {
            break;
        }

    }
    qDebug() << "end of frames";

runQuit:
    if (avioCtx) {
        av_freep(&avioCtx);
    }
    if (formatCtx && isFormatCtxOpen) {
        avformat_close_input(&formatCtx);
    }
    if (formatCtx) {
        avformat_free_context(formatCtx);
    }
    if (isCodecCtxOpen && codecCtx) {
        avcodec_close(codecCtx);
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
    }

    emit onDecodeStop();


}

void decoder::pushFrame()
{
    bool previousFrameConsumed = m_frames->offerDecodedFrame();
    //qDebug() << previousFrameConsumed;
    if (!previousFrameConsumed) {
        return;
    }
    emit onNewFrame();
}

