#pragma once

#include "CornerOverlay.h"
#include <QWidget>

class Panel : public QWidget
{
  public:
    Panel(QWidget* parent);
    void resizeEvent(QResizeEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void setBorderColor(QColor color);

  private:
    CornerOverlay* cornerOverlay_;
    QColor color;
};
