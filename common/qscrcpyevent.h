#ifndef QSCRCPYEVENT_H
#define QSCRCPYEVENT_H

#include <QEvent>

class qscrcpyevent : public QEvent
{
public:
    enum Type{
        DeviceSocket = QEvent::User + 1,
    };

public:
    qscrcpyevent(Type type);
};

//定义一个DeviceSocket事件类
class DeviceSocketEvent : public qscrcpyevent
{
public:
    DeviceSocketEvent() : qscrcpyevent(DeviceSocket) {

    };
};

#endif // QSCRCPYEVENT_H
