#include "Gui/Parameters/XYZParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include <iostream>

enzo::ui::XYZParm::XYZParm(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent)
    : Parameter(std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate(), parent),
      parameter_(parameter)
{
    auto parameterShared = parameter_.lock();

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

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
            0.0
        );
        contentLayout_->addWidget(slider);
        sliders_.push_back(slider);

        connect(slider, &Slider::sliderPressed, this, [this, vectorIndex]() {
            onPressed(vectorIndex);
        });
        connect(slider, &Slider::sliderMoved, this, [this, vectorIndex](double value) {
            onMoved(vectorIndex, value);
        });
        connect(slider, &Slider::sliderReleased, this, &XYZParm::onReleased);
        connect(
            slider,
            &Slider::expressionEntered,
            this,
            [this, vectorIndex](const QString& expression) {
                onExpressionEntered(vectorIndex, expression);
            }
        );
    }

    syncFromParameter();

    valueChangedConnection_ =
        parameterShared->valueChanged.connect([this]() { syncFromParameter(); });
}

void enzo::ui::XYZParm::syncFromParameter()
{
    if (auto parameterShared = parameter_.lock())
    {
        for (unsigned int vectorIndex = 0; vectorIndex < sliders_.size(); ++vectorIndex)
        {
            String error;
            sliders_[vectorIndex]->setValue(parameterShared->evalFloat(vectorIndex, error));

            // A component shows its expression source when one drives it, red
            // when it failed to evaluate.
            std::optional<String> expression = parameterShared->getExpression(vectorIndex);
            sliders_[vectorIndex]->setExpressionText(
                expression ? QString::fromStdString(*expression) : QString()
            );
            sliders_[vectorIndex]->setExpressionHasError(!error.empty());

            if (!error.empty())
                std::cerr << "Expression error on " << parameterShared->getName() << ": " << error
                          << "\n";
        }
    }
}

void enzo::ui::XYZParm::onExpressionEntered(unsigned int vectorIndex, const QString& expression)
{
    if (auto parameterShared = parameter_.lock())
    {
        parameterShared->setExpression(expression.toStdString(), vectorIndex);
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
        parameterShared->setFloat(static_cast<floatT>(value), vectorIndex);
    }
}

void enzo::ui::XYZParm::onReleased()
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
