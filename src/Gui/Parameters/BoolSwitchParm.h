#pragma once

#include "Engine/Parameter/Parameter.h"
#include "Gui/Parameters/Parameter.h"
#include "Gui/UtilWidgets/BoolSwitch.h"
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo::ui
{

class BoolSwitchParm
: public Parameter
{
    Q_OBJECT
public:
    BoolSwitchParm(std::weak_ptr<enzo::prm::Parameter> parameter, QWidget* parent = nullptr);

private:
    void onToggle(bool checked);
    void syncFromParameter();

    std::weak_ptr<prm::Parameter> parameter_;
    BoolSwitch* boolSwitch_ = nullptr;
    boost::signals2::scoped_connection valueChangedConnection_;
};

}
