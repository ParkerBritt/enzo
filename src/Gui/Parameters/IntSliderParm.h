#pragma once
#include "Engine/Parameter/Parameter.h"
#include "Engine/Types.h"
#include <QLabel>
#include <QPen>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/signals2/connection.hpp>

namespace enzo::ui {

class IntSliderParm : public QWidget {
    Q_OBJECT
  public:
    IntSliderParm(std::weak_ptr<enzo::prm::Parameter> parameter, QWidget *parent = nullptr,
                  Qt::WindowFlags f = Qt::WindowFlags());

  private:
    QVBoxLayout *mainLayout_;
    QLabel *valueLabel_;
    bt::intT value_;
    bool clampMin_;
    bool clampMax_;
    bt::intT minValue_;
    bt::intT maxValue_;

    std::weak_ptr<prm::Parameter> parameter_;

    QPen notchPen_;
    static constexpr int notchWidth = 2;

    void setValueImpl(bt::intT value);
    boost::signals2::scoped_connection valueChangedConnection_;

  protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

} // namespace enzo::ui

