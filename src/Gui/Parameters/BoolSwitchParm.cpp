#include "Gui/Parameters/BoolSwitchParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"

enzo::ui::BoolSwitchParm::BoolSwitchParm(
    std::weak_ptr<enzo::prm::NodeParameter> parameter,
    QWidget* parent
)
    : Parameter(std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate(), parent),
      parameter_(parameter)
{
    auto parameterShared = parameter_.lock();

    boolSwitch_ = new BoolSwitch(parameterShared->evalInt() != 0);
    contentLayout_->addWidget(boolSwitch_);
    contentLayout_->addStretch();
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    // Switch widgets never want the standard parameter background frame.
    contentWidget_->setStyleSheet(".ParameterBg { background: transparent; border: none; }");

    valueChangedConnection_ =
        parameterShared->valueChanged.connect([this]() { syncFromParameter(); });
    connect(boolSwitch_, &QPushButton::toggled, this, &BoolSwitchParm::onToggle);
}

void enzo::ui::BoolSwitchParm::syncFromParameter()
{
    if (auto parameterShared = parameter_.lock())
    {
        const bool toggled = parameterShared->evalInt() != 0;
        boolSwitch_->blockSignals(true);
        boolSwitch_->setChecked(toggled);
        boolSwitch_->blockSignals(false);
        boolSwitch_->animateSwitch(toggled);
    }
}

void enzo::ui::BoolSwitchParm::onToggle(bool checked)
{
    if (auto parameterShared = parameter_.lock())
    {
        auto before = parameterShared->getValues();
        parameterShared->setInt(checked);
        auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(
            parameterShared->getOpId(),
            parameterShared->getName(),
            before,
            parameterShared->getValues()
        );
        enzo::nt::nm().undoStack().push(std::move(cmd));
    }
}
