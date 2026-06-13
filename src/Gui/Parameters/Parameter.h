#pragma once

#include "Engine/Parameter/Template.h"
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <qtmetamacros.h>

class QEvent;

namespace enzo::ui {

class Parameter : public QWidget
{
    Q_OBJECT
  public:
    Parameter(const prm::Template& parmTemplate, QWidget* parent = nullptr);
    /// @brief Returns the name of the parameter this widget edits.
    enzo::String getName() const { return parmTemplate_.getName(); }
    int getLeftPadding();
    void setLeftPadding(int padding);
    virtual void addChild(Parameter* child);

  protected:
    /// @brief Permanently suppress the parameter frame for widgets that paint their own.
    /// @note This is a type level override, not the configurable Template background flag.
    void disableBackground();

    /// @brief Dims the whole row while the parameter is disabled.
    void changeEvent(QEvent* event) override;

    QHBoxLayout* mainLayout_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    QBoxLayout* contentLayout_ = nullptr;
    QLabel* label_ = nullptr;
    const prm::Template parmTemplate_;
};

} // namespace enzo::ui
