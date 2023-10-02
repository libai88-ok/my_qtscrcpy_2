#include "qscrcpyevent.h"


//在父类构造函数中一个强制转换
qscrcpyevent::qscrcpyevent(Type type)
    : QEvent(QEvent::Type(type))
{

}
