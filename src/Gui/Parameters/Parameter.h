#pragma once

#include "Engine/Parameter/Template.h"
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <qtmetamacros.h>

namespace enzo::ui {

class Parameter : public QWidget
{
    Q_OBJECT
  public:
    static constexpr int ROW_HEIGHT = 24;

    Parameter(const prm::Template& parmTemplate, QWidget* parent = nullptr);
    int getLeftPadding();
    void setLeftPadding(int padding);
    virtual void addChild(Parameter* child);

  protected:
    /// @brief Permanently suppress the parameter frame for widgets that paint their own.
    /// @note This is a type level override, not the configurable Template background flag.
    void disableBackground();

    QHBoxLayout* mainLayout_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    QBoxLayout* contentLayout_ = nullptr;
    QLabel* label_ = nullptr;
};

} // namespace enzo::ui
