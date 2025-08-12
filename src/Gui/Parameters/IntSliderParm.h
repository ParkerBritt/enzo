#pragma once
#include "Engine/Types.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPen>
#include "Engine/Parameter/Parameter.h"

namespace enzo::ui
{

class IntSliderParm
: public QWidget
{
    Q_OBJECT
public:
    IntSliderParm(std::weak_ptr<enzo::prm::Parameter> parameter, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    void setValue(bt::intT value);

Q_SIGNALS:
    void valueChanged(bt::intT value); 

private:

    QVBoxLayout* mainLayout_;
    QLabel* valueLabel_;
    bt::intT value_;
    bool clampMin_;
    bool clampMax_;
    bt::intT minValue_;
    bt::intT maxValue_;

    std::weak_ptr<prm::Parameter> parameter_;

    QPen notchPen_;
    static constexpr int notchWidth = 2;

    void setValueImpl(bt::intT value);

protected:
void paintEvent(QPaintEvent *event) override;
void mouseMoveEvent(QMouseEvent *event) override;
void mousePressEvent(QMouseEvent *event) override;

};

}

