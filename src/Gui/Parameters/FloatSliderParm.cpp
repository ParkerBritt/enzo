#include "Gui/Parameters/FloatSliderParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"

enzo::ui::FloatSliderParm::FloatSliderParm(std::weak_ptr<prm::Parameter> parameter,
                                           unsigned int vectorIndex, QWidget *parent)
: Parameter(std::shared_ptr<prm::Parameter>(parameter)->getTemplate(), parent),
  parameter_(parameter),
  vectorIndex_(vectorIndex)
{
    auto parameterShared = parameter_.lock();

    const auto range = parameterShared->getTemplate().getRange(vectorIndex_);
    slider_ = new Slider(
        range.getMin(),
        range.getMax(),
        range.getMinFlag() == prm::RangeFlag::LOCKED,
        range.getMaxFlag() == prm::RangeFlag::LOCKED,
        0.0);
    slider_->setValue(parameterShared->evalFloat(vectorIndex_));
    contentLayout_->addWidget(slider_);

    valueChangedConnection_ = parameterShared->valueChanged.connect([this]() {
        syncFromParameter();
    });

    connect(slider_, &Slider::sliderPressed, this, &FloatSliderParm::onPressed);
    connect(slider_, &Slider::sliderMoved, this, &FloatSliderParm::onMoved);
    connect(slider_, &Slider::sliderReleased, this, &FloatSliderParm::onReleased);
}

void enzo::ui::FloatSliderParm::syncFromParameter()
{
    if (auto parameterShared = parameter_.lock())
    {
        slider_->setValue(parameterShared->evalFloat(vectorIndex_));
    }
}

void enzo::ui::FloatSliderParm::onPressed()
{
    if (auto parameterShared = parameter_.lock())
    {
        valueBeforeDrag_ = parameterShared->getValues();
        undoDisabler_.emplace(UndoCommandType::ChangeParameter);
    }
}

void enzo::ui::FloatSliderParm::onMoved(double value)
{
    if (auto parameterShared = parameter_.lock())
    {
        parameterShared->setFloat(static_cast<bt::floatT>(value), vectorIndex_);
    }
}

void enzo::ui::FloatSliderParm::onReleased()
{
    undoDisabler_.reset();
    if (auto parameterShared = parameter_.lock())
    {
        auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(
            parameterShared->getOpId(), parameterShared->getName(), valueBeforeDrag_,
            parameterShared->getValues());
        enzo::nt::nm().undoStack().push(std::move(cmd));
    }
}
