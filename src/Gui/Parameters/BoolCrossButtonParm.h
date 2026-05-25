#pragma once

#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Parameter/Style.h"
#include "Gui/Parameters/Parameter.h"
#include <QPushButton>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo::ui
{

class BoolCrossButtonParm
: public Parameter
{
    Q_OBJECT
public:
    BoolCrossButtonParm(std::weak_ptr<prm::NodeParameter> parameter,
                        std::shared_ptr<prm::style::BoolCrossButton> style,
                        QWidget* parent = nullptr);

private:
    void onToggle(bool checked);
    void syncFromParameter();

    std::weak_ptr<prm::NodeParameter> parameter_;
    std::shared_ptr<prm::style::BoolCrossButton> style_;
    QPushButton* button_ = nullptr;
    boost::signals2::scoped_connection valueChangedConnection_;
};

}
