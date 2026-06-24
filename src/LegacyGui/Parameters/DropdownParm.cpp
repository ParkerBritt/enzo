#include "LegacyGui/Parameters/DropdownParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"

enzo::ui::DropdownParm::DropdownParm(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent)
    : Parameter(std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate(), parent),
      parameter_(parameter)
{
    auto parameterShared = parameter_.lock();

    dropdown_ = new Dropdown();
    for (const prm::Name& option : parameterShared->getTemplate().getOptions())
    {
        dropdown_->addItem(
            QString::fromStdString(option.getLabel()),
            QString::fromStdString(option.getToken())
        );
    }
    dropdown_->setCurrentData(QString::fromStdString(parameterShared->evalString()));
    contentLayout_->addWidget(dropdown_);

    // Stay at natural width so a horizontal group neighbour gets the leftover space.
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    // The dropdown paints its own rounded border, so the standard parameter
    // frame would draw a redundant box around it.
    disableBackground();

    valueChangedConnection_ =
        parameterShared->valueChanged.connect([this]() { syncFromParameter(); });
    connect(dropdown_, &Dropdown::currentIndexChanged, this, &DropdownParm::onSelect);
}

void enzo::ui::DropdownParm::onSelect()
{
    if (auto parameterShared = parameter_.lock())
    {
        auto before = parameterShared->getValues();
        parameterShared->setString(dropdown_->currentData().toStdString());
        auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(
            parameterShared->getOpId(),
            parameterShared->getName(),
            before,
            parameterShared->getValues()
        );
        enzo::nt::nm().undoStack().push(std::move(cmd));
    }
}

void enzo::ui::DropdownParm::syncFromParameter()
{
    if (auto parameterShared = parameter_.lock())
    {
        dropdown_->blockSignals(true);
        dropdown_->setCurrentData(QString::fromStdString(parameterShared->evalString()));
        dropdown_->blockSignals(false);
    }
}
