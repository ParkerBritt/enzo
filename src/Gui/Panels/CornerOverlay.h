#pragma once

#include <QPen>
#include <QWidget>

class CornerOverlay : public QWidget {
public:
    CornerOverlay(QWidget* parent);
    void setBorderColor(QColor color);

protected:
    void paintEvent(QPaintEvent*) override;
    QPen borderPen_ = QPen(QColor("#303030"));
};
