#include "Gui/Parameters/RampParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include "Gui/UtilWidgets/Ramp.h"
#include "Gui/UtilWidgets/Slider.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace {

// Pairs a caption with its slider on one row.
QHBoxLayout* sliderRow(const QString& label, enzo::ui::Slider* slider)
{
    QHBoxLayout* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    QLabel* caption = new QLabel(label);
    caption->setStyleSheet("QLabel{background: none}");
    caption->setFixedWidth(60);
    row->addWidget(caption);
    row->addWidget(slider);
    return row;
}

} // namespace

enzo::ui::RampParm::RampParm(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent)
    : Parameter(std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate(), parent),
      parameter_(parameter)
{
    QVBoxLayout* column = new QVBoxLayout();
    column->setContentsMargins(0, 0, 0, 0);

    rampWidget_ = new Ramp();
    column->addWidget(rampWidget_);

    indexSlider_ = new Slider(0, 0, true, true, 1.0);
    indexSlider_->setDisplayPrecision(0);
    positionSlider_ = new Slider(0, 1, true, true, 0.0);
    valueSlider_ = new Slider(0, 1, false, false, 0.0);

    column->addLayout(sliderRow("Point", indexSlider_));
    column->addLayout(sliderRow("Position", positionSlider_));
    column->addLayout(sliderRow("Value", valueSlider_));

    contentLayout_->addLayout(column);

    // The ramp widget paints its own frame, so the standard parameter
    // frame would draw a redundant box around it.
    disableBackground();

    connect(rampWidget_, &Ramp::edited, this, &RampParm::onRampEdited);
    connect(rampWidget_, &Ramp::selectionChanged, this, &RampParm::selectInstance);
    connect(rampWidget_, &Ramp::editBegan, this, &RampParm::beginEdit);
    connect(rampWidget_, &Ramp::editEnded, this, &RampParm::commitEdit);
    connect(indexSlider_, &Slider::sliderMoved, this, &RampParm::onIndexMoved);
    connect(positionSlider_, &Slider::sliderMoved, this, &RampParm::onPositionMoved);
    connect(valueSlider_, &Slider::sliderMoved, this, &RampParm::onValueMoved);

    // Position and value drags edit instance fields, so each drag is one undo step.
    connect(positionSlider_, &Slider::sliderPressed, this, &RampParm::beginEdit);
    connect(positionSlider_, &Slider::sliderReleased, this, &RampParm::commitEdit);
    connect(valueSlider_, &Slider::sliderPressed, this, &RampParm::beginEdit);
    connect(valueSlider_, &Slider::sliderReleased, this, &RampParm::commitEdit);

    if (auto parameterShared = parameter_.lock())
        valueChangedConnection_ =
            parameterShared->valueChanged.connect([this]() { syncFromParameter(); });

    syncFromParameter();
}

std::shared_ptr<enzo::prm::Parameter>
enzo::ui::RampParm::selectedField_(std::string_view fieldName) const
{
    auto parameterShared = parameter_.lock();
    if (!parameterShared) return nullptr;
    if (selectedInstance_ < 0 ||
        selectedInstance_ >= static_cast<int>(parameterShared->getInstanceCount()))
        return nullptr;
    return parameterShared->getInstanceField(selectedInstance_, fieldName);
}

void enzo::ui::RampParm::selectInstance(int instanceIndex)
{
    auto parameterShared = parameter_.lock();
    if (!parameterShared) return;

    const int count = static_cast<int>(parameterShared->getInstanceCount());
    selectedInstance_ = std::clamp(instanceIndex, 0, std::max(count - 1, 0));
    rampWidget_->setSelectedPoint(selectedInstance_);
    syncSelectedFields();
}

void enzo::ui::RampParm::onIndexMoved(double value)
{
    selectInstance(static_cast<int>(std::lround(value)));
}

void enzo::ui::RampParm::onPositionMoved(double value)
{
    if (auto field = selectedField_("position")) field->setFloat(static_cast<floatT>(value));
}

void enzo::ui::RampParm::onValueMoved(double value)
{
    if (auto field = selectedField_("value")) field->setFloat(static_cast<floatT>(value));
}

void enzo::ui::RampParm::beginEdit()
{
    if (auto parameterShared = parameter_.lock())
        snapshotBeforeEdit_ = toSerializable(*parameterShared);
}

void enzo::ui::RampParm::commitEdit()
{
    auto parameterShared = parameter_.lock();
    if (!parameterShared) return;

    ParameterSerializable after = toSerializable(*parameterShared);
    if (after == snapshotBeforeEdit_) return;

    auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(
        parameterShared->getOpId(),
        parameterShared->getName(),
        snapshotBeforeEdit_,
        after
    );
    enzo::nt::nm().undoStack().push(std::move(cmd));
}

void enzo::ui::RampParm::onRampEdited()
{
    auto parameterShared = parameter_.lock();
    if (!parameterShared) return;

    const std::vector<QPointF> points = rampWidget_->points();

    // The guard stops the echoed valueChanged from rebuilding the curve mid drag.
    applyingFromRamp_ = true;

    while (parameterShared->getInstanceCount() < points.size())
        parameterShared->addInstance();
    while (parameterShared->getInstanceCount() > points.size())
        parameterShared->removeInstance(parameterShared->getInstanceCount() - 1);

    for (unsigned int instanceIndex = 0; instanceIndex < points.size(); ++instanceIndex)
    {
        parameterShared->getInstanceField(instanceIndex, "position")
            ->setFloat(static_cast<floatT>(points[instanceIndex].x()));
        parameterShared->getInstanceField(instanceIndex, "value")
            ->setFloat(static_cast<floatT>(points[instanceIndex].y()));
    }

    applyingFromRamp_ = false;

    indexSlider_->setRange(0, std::max<int>(points.size() - 1, 0));
    syncSelectedFields();
}

void enzo::ui::RampParm::syncFromParameter()
{
    auto parameterShared = parameter_.lock();
    if (!parameterShared) return;

    const int count = static_cast<int>(parameterShared->getInstanceCount());
    indexSlider_->setRange(0, std::max(count - 1, 0));
    selectedInstance_ = std::clamp(selectedInstance_, 0, std::max(count - 1, 0));

    // A ramp edit already holds the curve, so only slider edits rebuild it here.
    if (!applyingFromRamp_)
    {
        std::vector<QPointF> points;
        points.reserve(count);
        for (int instanceIndex = 0; instanceIndex < count; ++instanceIndex)
            points.push_back(QPointF(
                parameterShared->getInstanceField(instanceIndex, "position")->evalFloat(),
                parameterShared->getInstanceField(instanceIndex, "value")->evalFloat()
            ));
        rampWidget_->setPoints(points);
        rampWidget_->setSelectedPoint(selectedInstance_);
    }

    syncSelectedFields();
}

void enzo::ui::RampParm::syncSelectedFields()
{
    indexSlider_->setValue(selectedInstance_);
    if (auto field = selectedField_("position")) positionSlider_->setValue(field->evalFloat());
    if (auto field = selectedField_("value")) valueSlider_->setValue(field->evalFloat());
}
