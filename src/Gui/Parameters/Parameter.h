#pragma once

#include "Engine/Parameter/Template.h"
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <qtmetamacros.h>

namespace enzo::ui
{

class Parameter
: public QWidget
{
    Q_OBJECT
public:
    static constexpr int ROW_HEIGHT = 24;

    Parameter(const prm::Template& parmTemplate, QWidget* parent = nullptr);
    int getLeftPadding();
    void setLeftPadding(int padding);
    virtual void addChild(Parameter* child);

protected:
    QHBoxLayout* mainLayout_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    QBoxLayout* contentLayout_ = nullptr;
    QLabel* label_ = nullptr;
};

}
