#include "Gui/Parameters/BoolCrossButtonParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include <QIcon>
#include <QSize>
#include <QString>

enzo::ui::BoolCrossButtonParm::BoolCrossButtonParm(
    std::weak_ptr<prm::NodeParameter> parameter,
    std::shared_ptr<prm::style::BoolCrossButton> style,
    QWidget* parent)
: Parameter(std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate(), parent),
  parameter_(parameter),
  style_(std::move(style))
{
    auto parameterShared = parameter_.lock();

    const int baseSize = 20;
    const int sizePx = static_cast<int>(baseSize * style_->scale());

    button_ = new QPushButton();
    button_->setCheckable(true);
    button_->setChecked(parameterShared->evalInt() != 0);
    button_->setIcon(QIcon(QString::fromStdString(style_->icon())));
    button_->setIconSize(QSize(sizePx, sizePx));
    button_->setFixedSize(sizePx + 8, sizePx + 8);
    button_->setFlat(true);

    contentLayout_->addWidget(button_);
    contentLayout_->addStretch();
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    valueChangedConnection_ = parameterShared->valueChanged.connect([this]() {
        syncFromParameter();
    });
    connect(button_, &QPushButton::toggled, this, &BoolCrossButtonParm::onToggle);
}

void enzo::ui::BoolCrossButtonParm::syncFromParameter()
{
    if (auto parameterShared = parameter_.lock())
    {
        const bool toggled = parameterShared->evalInt() != 0;
        button_->blockSignals(true);
        button_->setChecked(toggled);
        button_->blockSignals(false);
    }
}

void enzo::ui::BoolCrossButtonParm::onToggle(bool checked)
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
