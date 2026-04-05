#pragma once
#include "Engine/Parameter/Parameter.h"
#include "Engine/Types.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/signals2/connection.hpp>

namespace enzo::ui {

class FloatSliderParm : public QWidget {
    Q_OBJECT
  public:
    FloatSliderParm(std::weak_ptr<prm::Parameter> parameter, unsigned int vectorIndex = 0,
                    QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  private:
    unsigned int vectorIndex_; // index into the parameter's value vector, e.g. 0=X, 1=Y, 2=Z for XYZ types
    QVBoxLayout *mainLayout_;
    QLabel *valueLabel_;
    bt::floatT value_;
    bool clampMin_;
    bool clampMax_;
    bt::floatT minValue_;
    bt::floatT maxValue_;
    std::weak_ptr<prm::Parameter> parameter_;

    void setValueImpl(bt::floatT value);
    boost::signals2::scoped_connection valueChangedConnection_;

  protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

} // namespace enzo::ui
