#include "Gui/Parameters/XYZParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"

enzo::ui::XYZParm::XYZParm(std::weak_ptr<prm::Parameter> parameter, QWidget *parent)
: Parameter(std::shared_ptr<prm::Parameter>(parameter)->getTemplate(), parent),
  parameter_(parameter)
{
    auto parameterShared = parameter_.lock();

    const unsigned int vectorSize = parameterShared->getVectorSize();
    sliders_.reserve(vectorSize);
    for (unsigned int vectorIndex = 0; vectorIndex < vectorSize; ++vectorIndex)
    {
        const auto range = parameterShared->getTemplate().getRange(vectorIndex);
        Slider* slider = new Slider(
            range.getMin(),
            range.getMax(),
            range.getMinFlag() == prm::RangeFlag::LOCKED,
            range.getMaxFlag() == prm::RangeFlag::LOCKED,
            0.0);
        slider->setValue(parameterShared->evalFloat(vectorIndex));
        contentLayout_->addWidget(slider);
        sliders_.push_back(slider);

        connect(slider, &Slider::sliderPressed, this, [this, vectorIndex]() {
            onPressed(vectorIndex);
        });
        connect(slider, &Slider::sliderMoved, this, [this, vectorIndex](double value) {
            onMoved(vectorIndex, value);
        });
        connect(slider, &Slider::sliderReleased, this, &XYZParm::onReleased);
    }

    valueChangedConnection_ = parameterShared->valueChanged.connect([this]() {
        syncFromParameter();
    });
}

void enzo::ui::XYZParm::syncFromParameter()
{
    if (auto parameterShared = parameter_.lock())
    {
        for (unsigned int vectorIndex = 0; vectorIndex < sliders_.size(); ++vectorIndex)
        {
            sliders_[vectorIndex]->setValue(parameterShared->evalFloat(vectorIndex));
        }
    }
}

void enzo::ui::XYZParm::onPressed(unsigned int)
{
    if (auto parameterShared = parameter_.lock())
    {
        valueBeforeDrag_ = parameterShared->getValues();
        undoDisabler_.emplace(UndoCommandType::ChangeParameter);
    }
}

void enzo::ui::XYZParm::onMoved(unsigned int vectorIndex, double value)
{
    if (auto parameterShared = parameter_.lock())
    {
        parameterShared->setFloat(static_cast<bt::floatT>(value), vectorIndex);
    }
}

void enzo::ui::XYZParm::onReleased()
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
