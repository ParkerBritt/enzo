#include "LegacyGui/Parameters/StringParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include "LegacyGui/Style.h"

enzo::ui::StringParm::StringParm(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent)
    : Parameter(std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate(), parent),
      parameter_(parameter)
{
    auto parameterShared = parameter_.lock();

    lineEdit_ = new QLineEdit();
    lineEdit_->setFixedHeight(enzo::style::parameter::height);
    lineEdit_->setText(QString::fromStdString(parameterShared->evalString()));
    lineEdit_->setStyleSheet(
        "QLineEdit { background: transparent; border: none; padding: 0 5px; }"
    );
    contentLayout_->addWidget(lineEdit_);

    valueChangedConnection_ =
        parameterShared->valueChanged.connect([this]() { syncFromParameter(); });

    connect(lineEdit_, &QLineEdit::textEdited, this, &StringParm::setValueQString);
    connect(lineEdit_, &QLineEdit::editingFinished, this, &StringParm::onEditingFinished);
}

void enzo::ui::StringParm::setValueQString(QString value)
{
    if (auto parameterShared = parameter_.lock())
    {
        if (!undoDisabler_)
        {
            valueBeforeEdit_ = parameterShared->getValues();
            undoDisabler_.emplace(UndoCommandType::ChangeParameter);
        }
        parameterShared->setString(value.toStdString());
    }
}

void enzo::ui::StringParm::syncFromParameter()
{
    if (auto parameterShared = parameter_.lock())
    {
        if (!lineEdit_->hasFocus())
        {
            lineEdit_->setText(QString::fromStdString(parameterShared->evalString()));
        }
    }
}

void enzo::ui::StringParm::onEditingFinished()
{
    undoDisabler_.reset();
    if (auto parameterShared = parameter_.lock())
    {
        auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(
            parameterShared->getOpId(),
            parameterShared->getName(),
            valueBeforeEdit_,
            parameterShared->getValues()
        );
        enzo::nt::nm().undoStack().push(std::move(cmd));
    }
}
