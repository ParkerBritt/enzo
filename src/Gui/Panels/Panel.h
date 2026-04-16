#pragma once

#include <QWidget>
#include "CornerOverlay.h"

class Panel
: public QWidget
{
public:
    Panel(QWidget *parent);
    void resizeEvent(QResizeEvent *event) override;
    void setBorderColor(QColor color);
private:
    CornerOverlay* cornerOverlay_;
    QColor color;
};
