#pragma once

#include "LegacyGui/Style.h"
#include <QPen>
#include <QWidget>

class CornerOverlay : public QWidget
{
  public:
    CornerOverlay(QWidget* parent);
    void setBorderColor(QColor color);

  protected:
    void paintEvent(QPaintEvent*) override;
    QPen borderPen_ = QPen(enzo::style::color::divider);
};
