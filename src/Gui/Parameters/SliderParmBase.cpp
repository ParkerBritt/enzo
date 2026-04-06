#include "Gui/Parameters/SliderParmBase.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include <QMouseEvent>

enzo::ui::SliderParmBase::SliderParmBase(std::weak_ptr<prm::Parameter> parameter,
                                         unsigned int vectorIndex, QWidget *parent,
                                         Qt::WindowFlags f)
    : QWidget(parent, f), vectorIndex_(vectorIndex), parameter_(parameter) {
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(24);

    mainLayout_ = new QVBoxLayout();
    mainLayout_->setContentsMargins(0, 0, 0, 0);
    setLayout(mainLayout_);

    valueLabel_ = new QLabel();
    valueLabel_->setAlignment(Qt::AlignCenter);
    valueLabel_->setStyleSheet("background-color: none;");
    setProperty("type", "SliderParm");
    mainLayout_->addWidget(valueLabel_);

    if (auto parameterShared = parameter_.lock()) {
        auto range = parameterShared->getTemplate().getRange(vectorIndex);
        minValue_ = range.getMin();
        maxValue_ = range.getMax();
        clampMin_ = range.getMinFlag() == prm::RangeFlag::LOCKED;
        clampMax_ = range.getMaxFlag() == prm::RangeFlag::LOCKED;

        valueChangedConnection_ = parameterShared->valueChanged.connect([this]() {
            syncFromParameter();
        });
    } else {
        throw std::bad_weak_ptr();
    }
}

void enzo::ui::SliderParmBase::mousePressEvent(QMouseEvent *event) {
    if (auto parameterShared = parameter_.lock()) {
        valueBeforeDrag_ = parameterShared->getValues();
        undoDisabler_.emplace(UndoCommandType::ChangeParameter);

        float normalized = static_cast<float>(event->pos().x()) / rect().width();
        applyValue(normalized);
    }
}

void enzo::ui::SliderParmBase::mouseMoveEvent(QMouseEvent *event) {
    float normalized = static_cast<float>(event->pos().x()) / rect().width();
    applyValue(normalized);
}

void enzo::ui::SliderParmBase::mouseReleaseEvent(QMouseEvent *event) {
    undoDisabler_.reset();
    if (auto parameterShared = parameter_.lock()) {
        auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(
            parameterShared->getOpId(), parameterShared->getName(), valueBeforeDrag_,
            parameterShared->getValues());
        enzo::nt::nm().undoStack().push(std::move(cmd));
    }
}
