#pragma once
#include "Engine/Parameter/Parameter.h"
#include "Engine/UndoRedo/UndoDisabler.h"
#include "Gui/Parameters/Parameter.h"
#include <QLineEdit>
#include <QWidget>
#include <boost/signals2/connection.hpp>
#include <optional>

namespace enzo::ui {

class StringParm : public Parameter {
    Q_OBJECT
  public:
    StringParm(std::weak_ptr<prm::Parameter> parameter, QWidget *parent = nullptr);

  private:
    void setValueQString(QString value);
    void onEditingFinished();
    void syncFromParameter();

    std::weak_ptr<prm::Parameter> parameter_;
    QLineEdit* lineEdit_ = nullptr;
    boost::signals2::scoped_connection valueChangedConnection_;
    std::optional<UndoDisabler> undoDisabler_;
    prm::PrmValues valueBeforeEdit_;
};

} // namespace enzo::ui
