#include "Gui/Parameters/BoolIconParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include "Gui/IconRegistry.h"
#include <QSize>

enzo::ui::BoolIconParm::BoolIconParm(
    std::weak_ptr<prm::NodeParameter> parameter,
    std::shared_ptr<prm::style::BoolIcon> style,
    QWidget* parent)
: Parameter(std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate(), parent),
  parameter_(parameter),
  style_(std::move(style))
{
    auto parameterShared = parameter_.lock();

    const int sizePx = ROW_HEIGHT;
    const int iconPx = sizePx * style_->scale()-8;

    button_ = new QPushButton();
    button_->setCheckable(true);
    button_->setChecked(parameterShared->evalInt() != 0);
    button_->setIcon(IconRegistry::instance().lookup(style_->icon()));
    button_->setIconSize(QSize(iconPx, iconPx));
    button_->setFixedSize(sizePx, sizePx);
    button_->setFlat(true);

    contentLayout_->addWidget(button_);
    contentLayout_->addStretch();
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    valueChangedConnection_ = parameterShared->valueChanged.connect([this]() {
        syncFromParameter();
    });
    connect(button_, &QPushButton::toggled, this, &BoolIconParm::onToggle);
}

void enzo::ui::BoolIconParm::syncFromParameter()
{
    if (auto parameterShared = parameter_.lock())
    {
        const bool toggled = parameterShared->evalInt() != 0;
        button_->blockSignals(true);
        button_->setChecked(toggled);
        button_->blockSignals(false);
    }
}

void enzo::ui::BoolIconParm::onToggle(bool checked)
{
    if (auto parameterShared = parameter_.lock())
    {
        auto before = parameterShared->getValues();
        parameterShared->setInt(checked);
        auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(
            parameterShared->getOpId(), parameterShared->getName(), before,
            parameterShared->getValues());
        enzo::nt::nm().undoStack().push(std::move(cmd));
    }
}
