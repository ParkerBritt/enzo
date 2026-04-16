#pragma once
#include "Engine/Parameter/Parameter.h"
#include "Engine/Types.h"
#include "Engine/UndoRedo/UndoDisabler.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/signals2/connection.hpp>
#include <optional>

namespace enzo::ui {

class SliderParmBase : public QWidget {
    Q_OBJECT
  public:
    SliderParmBase(std::weak_ptr<prm::Parameter> parameter, unsigned int vectorIndex = 0,
                   QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  protected:
    unsigned int vectorIndex_;
    QVBoxLayout *mainLayout_;
    QLabel *valueLabel_;
    bt::floatT minValue_;
    bt::floatT maxValue_;
    bool clampMin_;
    bool clampMax_;
    std::weak_ptr<prm::Parameter> parameter_;
    boost::signals2::scoped_connection valueChangedConnection_;

    // Subclasses implement these to read/write the parameter and update the label
    virtual void syncFromParameter() = 0;
    virtual void applyValue(float normalizedValue) = 0;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

  private:
    std::optional<UndoDisabler> undoDisabler_;
    prm::PrmValues valueBeforeDrag_;
};

} // namespace enzo::ui
