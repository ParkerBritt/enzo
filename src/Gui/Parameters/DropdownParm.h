#pragma once

#include "Engine/Parameter/NodeParameter.h"
#include "Gui/Parameters/Parameter.h"
#include "Gui/UtilWidgets/Dropdown.h"
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo::ui {

class DropdownParm : public Parameter
{
    Q_OBJECT
  public:
    DropdownParm(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent = nullptr);

  private:
    void onSelect();
    void syncFromParameter();

    std::weak_ptr<prm::NodeParameter> parameter_;
    Dropdown* dropdown_ = nullptr;
    boost::signals2::scoped_connection valueChangedConnection_;
};

} // namespace enzo::ui
