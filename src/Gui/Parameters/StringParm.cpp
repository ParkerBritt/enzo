#include "Gui/Parameters/StringParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"

enzo::ui::StringParm::StringParm(std::weak_ptr<prm::Parameter> parameter, QWidget *parent)
    : QLineEdit(parent), parameter_(parameter) {
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(24);

    if (auto parameterShared = parameter_.lock()) {
        setText(QString::fromStdString(parameterShared->evalString()));
        valueChangedConnection_ = parameterShared->valueChanged.connect([this]() {
            syncFromParameter();
        });
    } else {
        throw std::bad_weak_ptr();
    }

    setProperty("type", "StringParm");
    setStyleSheet(R"(
                  QWidget[type="StringParm"]
                  {
                      border-radius: 6px;
                      border: 1px solid #383838;
                      padding: 0px 5px 0px 5px;
                  }
                  )");

    connect(this, &QLineEdit::textEdited, this, &StringParm::setValueQString);
    connect(this, &QLineEdit::editingFinished, this, &StringParm::onEditingFinished);
}

void enzo::ui::StringParm::setValueQString(QString value) {
    if (auto parameterShared = parameter_.lock()) {
        if (!undoDisabler_) {
            valueBeforeEdit_ = parameterShared->getValues();
            undoDisabler_.emplace(UndoCommandType::ChangeParameter);
        }
        parameterShared->setString(value.toStdString());
    }
}

void enzo::ui::StringParm::syncFromParameter() {
    if (auto parameterShared = parameter_.lock()) {
        if (!hasFocus()) {
            setText(QString::fromStdString(parameterShared->evalString()));
        }
    }
}

void enzo::ui::StringParm::onEditingFinished() {
    undoDisabler_.reset();
    if (auto parameterShared = parameter_.lock()) {
        auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(
            parameterShared->getOpId(), parameterShared->getName(), valueBeforeEdit_,
            parameterShared->getValues());
        enzo::nt::nm().undoStack().push(std::move(cmd));
    }
}
