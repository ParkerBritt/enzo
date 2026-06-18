#include "Gui/Parameters/IntSliderParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include <cmath>
#include <iostream>

enzo::ui::IntSliderParm::IntSliderParm(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent)
    : Parameter(std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate(), parent),
      parameter_(parameter)
{
    auto parameterShared = parameter_.lock();

    const auto range = parameterShared->getTemplate().getRange(0);
    slider_ = new Slider(
        range.getMin(),
        range.getMax(),
        range.getMinFlag() == prm::RangeFlag::LOCKED,
        range.getMaxFlag() == prm::RangeFlag::LOCKED,
        1.0
    );
    slider_->setDisplayPrecision(0);
    contentLayout_->addWidget(slider_);
    syncFromParameter();

    valueChangedConnection_ =
        parameterShared->valueChanged.connect([this]() { syncFromParameter(); });

    connect(slider_, &Slider::sliderPressed, this, &IntSliderParm::onPressed);
    connect(slider_, &Slider::sliderMoved, this, &IntSliderParm::onMoved);
    connect(slider_, &Slider::sliderReleased, this, &IntSliderParm::onReleased);
    connect(slider_, &Slider::expressionEntered, this, &IntSliderParm::onExpressionEntered);
}

void enzo::ui::IntSliderParm::syncFromParameter()
{
    if (auto parameterShared = parameter_.lock())
    {
        String error;
        slider_->setValue(static_cast<double>(parameterShared->evalInt(0, error)));

        // The slider shows the expression source when one drives the value, red
        // when it failed to evaluate.
        std::optional<String> expression = parameterShared->getExpression(0);
        slider_->setExpressionText(expression ? QString::fromStdString(*expression) : QString());
        slider_->setExpressionHasError(!error.empty());

        if (!error.empty())
            std::cerr << "Expression error on " << parameterShared->getName() << ": " << error
                      << "\n";
    }
}

void enzo::ui::IntSliderParm::onExpressionEntered(const QString& expression)
{
    if (auto parameterShared = parameter_.lock())
    {
        parameterShared->setExpression(expression.toStdString());
    }
}

void enzo::ui::IntSliderParm::onPressed()
{
    if (auto parameterShared = parameter_.lock())
    {
        valueBeforeDrag_ = parameterShared->getValues();
        undoDisabler_.emplace(UndoCommandType::ChangeParameter);
    }
}

void enzo::ui::IntSliderParm::onMoved(double value)
{
    if (auto parameterShared = parameter_.lock())
    {
        parameterShared->setInt(static_cast<intT>(std::lround(value)));
    }
}

void enzo::ui::IntSliderParm::onReleased()
{
    undoDisabler_.reset();
    if (auto parameterShared = parameter_.lock())
    {
        auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(
            parameterShared->getOpId(),
            parameterShared->getName(),
            valueBeforeDrag_,
            parameterShared->getValues()
        );
        enzo::nt::nm().undoStack().push(std::move(cmd));
    }
}
