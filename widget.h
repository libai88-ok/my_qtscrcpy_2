#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include "server/server.h"
#include "decoder.h"
#include "frames.h"
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_startserver_clicked();

    void on_stopserver_clicked();

private:
    Ui::Widget *ui;

    server m_server;
    decoder m_decoder;
    Frames m_frames;
};
#endif // WIDGET_H
