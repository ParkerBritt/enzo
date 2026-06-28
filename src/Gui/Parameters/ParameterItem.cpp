#include "Gui/Parameters/ParameterItem.h"
#include "Engine/Core/Types.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Parameter/Template.h"

namespace enzo::ui {

namespace {

/// @brief Whether the type carries a numeric range worth reading.
bool hasRange(prm::Type type)
{
    return type == prm::Type::FLOAT || type == prm::Type::INT || type == prm::Type::XYZ;
}

} // namespace

ParameterItem::ParameterItem(
    const prm::Template& prmTemplate,
    std::weak_ptr<prm::NodeParameter> parameter,
    QObject* parent
)
    : QObject(parent), parameter_(std::move(parameter))
{
    kind_ = QString::fromStdString(prm::toString(prmTemplate.getType()));
    name_ = QString::fromStdString(prmTemplate.getName());
    label_ = QString::fromStdString(prmTemplate.getLabel());
    tooltip_ = QString::fromStdString(prmTemplate.getTooltip());
    vectorSize_ = static_cast<int>(prmTemplate.getSize());
    horizontal_ = prmTemplate.getDirection() == prm::Direction::HORIZONTAL;
    labelHidden_ = prmTemplate.isLabelHidden();

    if (hasRange(prmTemplate.getType()))
    {
        const prm::Range& range = prmTemplate.getRange(0);
        minimum_ = range.getMin();
        maximum_ = range.getMax();
        minLocked_ = range.getMinFlag() == prm::RangeFlag::LOCKED;
        maxLocked_ = range.getMaxFlag() == prm::RangeFlag::LOCKED;
    }

    for (const prm::Name& option : prmTemplate.getOptions())
    {
        options_.append(QString::fromStdString(option.getLabel()));
        optionTokens_.append(QString::fromStdString(option.getToken()));
    }

    // An undo or expression edit changes the value behind QML's back, so mirror
    // the engine parameter's own change signal out as the QML notify.
    if (auto param = parameter_.lock())
        valueSubscription_ = param->valueChanged.connect([this] { Q_EMIT valueChanged(); });
}

QVariant ParameterItem::valueAt(int index) const
{
    auto param = parameter_.lock();
    if (!param) return {};

    switch (param->getValueType())
    {
    case prm::ValueType::Float:
        return static_cast<double>(param->evalFloat(index));
    case prm::ValueType::Int:
        return static_cast<qlonglong>(param->evalInt(index));
    case prm::ValueType::String:
        return QString::fromStdString(param->evalString(index));
    }
    return {};
}

void ParameterItem::setValueAt(int index, const QVariant& value)
{
    auto param = parameter_.lock();
    if (!param) return;

    switch (param->getValueType())
    {
    case prm::ValueType::Float:
        param->setFloat(value.toDouble(), index);
        break;
    case prm::ValueType::Int:
        param->setInt(value.toLongLong(), index);
        break;
    case prm::ValueType::String:
        param->setString(value.toString().toStdString(), index);
        break;
    }
}

void ParameterItem::setMeta(bool enabled, bool hidden)
{
    if (enabled_ == enabled && hidden_ == hidden) return;
    enabled_ = enabled;
    hidden_ = hidden;
    Q_EMIT metaChanged();
}

} // namespace enzo::ui
