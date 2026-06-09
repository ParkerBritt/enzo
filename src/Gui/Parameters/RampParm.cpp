#include "Gui/Parameters/RampParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include "Gui/UtilWidgets/Dropdown.h"
#include "Gui/UtilWidgets/IconButton.h"
#include "Gui/UtilWidgets/PopupList.h"
#include "Gui/UtilWidgets/Ramp.h"
#include "Gui/UtilWidgets/Slider.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace {

// Pairs a caption with its field widget on one row.
QHBoxLayout* fieldRow(const QString& label, QWidget* field)
{
    QHBoxLayout* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    QLabel* caption = new QLabel(label);
    caption->setStyleSheet("QLabel{background: none}");
    caption->setFixedWidth(60);
    row->addWidget(caption);
    row->addWidget(field);
    return row;
}

} // namespace

enzo::ui::RampParm::RampParm(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent)
    : Parameter(std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate(), parent),
      parameter_(parameter)
{
    QVBoxLayout* column = new QVBoxLayout();
    column->setContentsMargins(0, 0, 0, 0);

    // Tool strip above the curve. The spline button picks one interp for every
    // point, the arrows flip the curve across each axis.
    interpButton_ = new IconButton("spline");
    flipHorizontalButton_ = new IconButton("arrow-left-right");
    flipVerticalButton_ = new IconButton("arrow-down-up");
    addPointButton_ = new IconButton("plus");
    deletePointButton_ = new IconButton("x");
    interpButton_->setToolTip("Set interpolation for all points");
    flipHorizontalButton_->setToolTip("Flip left to right");
    flipVerticalButton_->setToolTip("Flip up and down");
    addPointButton_->setToolTip("Add a point after the selected one");
    deletePointButton_->setToolTip("Delete the selected point");

    const int panelInset = static_cast<int>(std::lround(Ramp::panelInset));

    QHBoxLayout* toolRow = new QHBoxLayout();
    // Inset both ends so the strip lines up with the curve panel below. Add and
    // delete sit on the left, the interp and flip tools on the right.
    toolRow->setContentsMargins(panelInset, 0, panelInset, 0);
    toolRow->setSpacing(2);
    toolRow->addWidget(deletePointButton_);
    toolRow->addWidget(addPointButton_);
    toolRow->addStretch();
    toolRow->addWidget(interpButton_);
    toolRow->addWidget(flipHorizontalButton_);
    toolRow->addWidget(flipVerticalButton_);

    rampWidget_ = new Ramp();

    // Group the strip with the curve so they sit flush with no gap between them.
    QVBoxLayout* curveBlock = new QVBoxLayout();
    curveBlock->setContentsMargins(0, 0, 0, 0);
    curveBlock->setSpacing(0);
    curveBlock->addLayout(toolRow);
    curveBlock->addWidget(rampWidget_);
    column->addLayout(curveBlock);

    indexSlider_ = new Slider(0, 0, true, true, 1.0);
    indexSlider_->setDisplayPrecision(0);
    positionSlider_ = new Slider(0, 1, true, true, 0.0);
    valueSlider_ = new Slider(0, 1, false, false, 0.0);

    interpDropdown_ = new Dropdown();
    // Populate dropdown with options
    const prm::Template& interpTemplate =
        std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate().getChild("interp");
    for (const prm::Name& option : interpTemplate.getOptions())
        interpDropdown_->addItem(
            QString::fromStdString(option.getLabel()),
            QString::fromStdString(option.getToken())
        );

    // The interp popup carries the same options in the same order, so a chosen
    // row maps straight onto the Interpolation enum.
    interpPopup_ = new PopupList(this);
    for (const prm::Name& option : interpTemplate.getOptions())
        interpPopup_->addItem(QString::fromStdString(option.getLabel()));

    column->addLayout(fieldRow("Point", indexSlider_));
    column->addLayout(fieldRow("Position", positionSlider_));
    column->addLayout(fieldRow("Value", valueSlider_));
    column->addLayout(fieldRow("Interp", interpDropdown_));

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
    connect(interpDropdown_, &Dropdown::currentIndexChanged, this, &RampParm::onInterpChanged);

    connect(interpButton_, &IconButton::clicked, this, &RampParm::openInterpPopup);
    connect(flipHorizontalButton_, &IconButton::clicked, this, &RampParm::flipHorizontal);
    connect(flipVerticalButton_, &IconButton::clicked, this, &RampParm::flipVertical);
    connect(addPointButton_, &IconButton::clicked, this, &RampParm::addPoint);
    connect(deletePointButton_, &IconButton::clicked, this, &RampParm::deletePoint);
    connect(interpPopup_, &PopupList::itemSelected, this, [this](int index) {
        setAllInterps(static_cast<prm::Interpolation>(index));
    });

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

void enzo::ui::RampParm::onInterpChanged()
{
    auto field = selectedField_("interp");
    if (!field) return;

    // An interp change is a single edit, so snapshot and commit it as one undo step.
    beginEdit();
    field->setString(interpDropdown_->currentData().toStdString());
    commitEdit();
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

void enzo::ui::RampParm::applyControlPoints(
    const std::vector<QPointF>& points,
    const std::vector<prm::Interpolation>& interps
)
{
    beginEdit();
    rampWidget_->setPoints(points, interps);
    rampWidget_->setSelectedPoint(selectedInstance_);
    // The curve is the source of truth here, so reuse the same write path a drag uses.
    onRampEdited();
    commitEdit();
}

void enzo::ui::RampParm::flipHorizontal()
{
    const std::vector<QPointF> points = rampWidget_->points();
    const std::vector<prm::Interpolation> interps = rampWidget_->interps();
    if (points.empty()) return;

    // Mirror positions across the centre of the domain. A point's interp governs
    // the segment to its right, so reversing the order hands each segment's interp
    // to the point now on its left.
    std::vector<QPointF> flippedPoints(points.size());
    std::vector<prm::Interpolation> flippedInterps(points.size());
    for (std::size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex)
    {
        flippedPoints[pointIndex] = QPointF(1.0 - points[pointIndex].x(), points[pointIndex].y());
        flippedInterps[pointIndex] = pointIndex == 0 ? interps[0] : interps[pointIndex - 1];
    }

    applyControlPoints(flippedPoints, flippedInterps);
}

void enzo::ui::RampParm::flipVertical()
{
    const std::vector<QPointF> points = rampWidget_->points();
    const std::vector<prm::Interpolation> interps = rampWidget_->interps();
    if (points.empty()) return;

    // Mirror values across the centre of the visible band so the whole curve
    // turns upside down. The band always spans at least zero to one.
    double minValue = 0.0;
    double maxValue = 1.0;
    for (const QPointF& point : points)
    {
        minValue = std::min(minValue, point.y());
        maxValue = std::max(maxValue, point.y());
    }

    std::vector<QPointF> flippedPoints(points.size());
    for (std::size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex)
        flippedPoints[pointIndex] =
            QPointF(points[pointIndex].x(), minValue + maxValue - points[pointIndex].y());

    applyControlPoints(flippedPoints, interps);
}

void enzo::ui::RampParm::openInterpPopup()
{
    const QPoint topLeft = interpButton_->mapToGlobal(QPoint(0, interpButton_->height() + 2));
    interpPopup_->open(topLeft, 140);
}

void enzo::ui::RampParm::setAllInterps(prm::Interpolation interp)
{
    const std::vector<QPointF> points = rampWidget_->points();
    if (points.empty()) return;

    const std::vector<prm::Interpolation> interps(points.size(), interp);
    applyControlPoints(points, interps);
}

void enzo::ui::RampParm::addPoint()
{
    std::vector<QPointF> points = rampWidget_->points();
    std::vector<prm::Interpolation> interps = rampWidget_->interps();
    const int count = static_cast<int>(points.size());
    if (count == 0) return;

    const int selected = std::clamp(selectedInstance_, 0, count - 1);

    // Split the segment to the right of the selected point, falling back to the
    // one on its left when it is already the last point.
    int leftIndex = selected;
    int rightIndex = selected + 1;
    if (rightIndex >= count)
    {
        leftIndex = selected - 1;
        rightIndex = selected;
    }

    QPointF newPoint;
    prm::Interpolation newInterp;
    if (leftIndex < 0)
    {
        // A lone point has no neighbour, so the new one lands halfway to the right edge.
        newPoint = QPointF((points[selected].x() + 1.0) / 2.0, points[selected].y());
        newInterp = interps[selected];
        leftIndex = selected;
    }
    else
    {
        // The new point inherits the left interp so the split keeps the segment shape.
        newPoint = (points[leftIndex] + points[rightIndex]) / 2.0;
        newInterp = interps[leftIndex];
    }

    points.push_back(newPoint);
    interps.push_back(newInterp);

    // Sorting by position drops the new point right after everything left of it.
    selectedInstance_ = leftIndex + 1;
    applyControlPoints(points, interps);
}

void enzo::ui::RampParm::deletePoint()
{
    std::vector<QPointF> points = rampWidget_->points();
    std::vector<prm::Interpolation> interps = rampWidget_->interps();
    const int count = static_cast<int>(points.size());

    // Keep at least one point so the ramp always has a value.
    if (count <= 1) return;

    const int selected = std::clamp(selectedInstance_, 0, count - 1);
    points.erase(points.begin() + selected);
    interps.erase(interps.begin() + selected);

    selectedInstance_ = std::clamp(selected, 0, count - 2);
    applyControlPoints(points, interps);
}

void enzo::ui::RampParm::onRampEdited()
{
    auto parameterShared = parameter_.lock();
    if (!parameterShared) return;

    const std::vector<QPointF> points = rampWidget_->points();
    const std::vector<prm::Interpolation> interps = rampWidget_->interps();

    // The guard stops the echoed valueChanged from rebuilding the curve mid drag.
    applyingFromRamp_ = true;

    {
        // Each field write dirties the node, which on its own would recook the
        // curve once per point. Holding one update lock over the whole rewrite
        // coalesces them into a single cook when the lock releases.
        auto updateLock = enzo::nt::nm().lockUpdates();

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
            parameterShared->getInstanceField(instanceIndex, "interp")
                ->setInt(static_cast<intT>(interps[instanceIndex]));
        }
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
        std::vector<prm::Interpolation> interps;
        points.reserve(count);
        interps.reserve(count);
        for (int instanceIndex = 0; instanceIndex < count; ++instanceIndex)
        {
            points.push_back(QPointF(
                parameterShared->getInstanceField(instanceIndex, "position")->evalFloat(),
                parameterShared->getInstanceField(instanceIndex, "value")->evalFloat()
            ));
            const intT interpIndex =
                parameterShared->getInstanceField(instanceIndex, "interp")->evalInt();
            interps.push_back(static_cast<prm::Interpolation>(interpIndex));
        }
        rampWidget_->setPoints(points, interps);
        rampWidget_->setSelectedPoint(selectedInstance_);
    }

    syncSelectedFields();
}

void enzo::ui::RampParm::syncSelectedFields()
{
    indexSlider_->setValue(selectedInstance_);
    if (auto field = selectedField_("position")) positionSlider_->setValue(field->evalFloat());
    if (auto field = selectedField_("value")) valueSlider_->setValue(field->evalFloat());
    if (auto field = selectedField_("interp"))
    {
        interpDropdown_->blockSignals(true);
        interpDropdown_->setCurrentData(QString::fromStdString(field->evalString()));
        interpDropdown_->blockSignals(false);
    }
}
