#include "Gui/Parameters/ParameterItem.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Parameter/Template.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"

namespace enzo::ui {

namespace {

/// @brief Whether the type carries a numeric range worth reading.
bool hasRange(prm::Type type)
{
    return type == prm::Type::FLOAT || type == prm::Type::INT || type == prm::Type::XYZ;
}

/// @brief Returns a multiparm's instances as a list of maps keyed by field name.
///
/// e.g. a ramp reads as
/// [{position 0, value 0, interp "linear"}, {position 1, value 1, interp "linear"}]
QVariantList getInstanceValues(const prm::Parameter& param)
{
    QVariantList instances;
    for (unsigned int instanceIndex = 0; instanceIndex < param.getInstanceCount(); ++instanceIndex)
    {
        QVariantMap fields;
        for (const std::shared_ptr<prm::Parameter>& field : param.getInstance(instanceIndex))
        {
            const QString fieldName = QString::fromStdString(field->getName());
            switch (field->getValueType())
            {
            case prm::ValueType::Float:
                fields[fieldName] = static_cast<double>(field->evalFloat());
                break;
            case prm::ValueType::Int:
                fields[fieldName] = static_cast<qlonglong>(field->evalInt());
                break;
            case prm::ValueType::String:
                fields[fieldName] = QString::fromStdString(field->evalString());
                break;
            }
        }
        instances.append(fields);
    }
    return instances;
}

/// @brief Rebuilds a multiparm's instances from a list of maps keyed by field name.
/// @note A field the map leaves out keeps whatever it already held.
void setInstanceValues(prm::Parameter& param, const QVariantList& instances)
{
    // One update lock coalesces the per field cooks into a single recook.
    auto updateLock = nt::nm().lockUpdates();

    while (param.getInstanceCount() < static_cast<unsigned int>(instances.size()))
        param.addInstance();
    while (param.getInstanceCount() > static_cast<unsigned int>(instances.size()))
        param.removeInstance(param.getInstanceCount() - 1);

    for (int instanceIndex = 0; instanceIndex < instances.size(); ++instanceIndex)
    {
        const QVariantMap fields = instances[instanceIndex].toMap();
        for (const std::shared_ptr<prm::Parameter>& field : param.getInstance(instanceIndex))
        {
            const QString fieldName = QString::fromStdString(field->getName());
            if (!fields.contains(fieldName)) continue;

            const QVariant fieldValue = fields.value(fieldName);
            switch (field->getValueType())
            {
            case prm::ValueType::Float:
                field->setFloat(static_cast<floatT>(fieldValue.toDouble()));
                break;
            case prm::ValueType::Int:
                field->setInt(static_cast<intT>(fieldValue.toLongLong()));
                break;
            case prm::ValueType::String:
                field->setString(fieldValue.toString().toStdString());
                break;
            }
        }
    }
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

    if (param->getTemplate().isMultiParm()) return getInstanceValues(*param);

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

    if (param->getTemplate().isMultiParm())
    {
        setInstanceValues(*param, value.toList());
        return;
    }

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

bool ParameterItem::hasExpressionAt(int index) const
{
    auto param = parameter_.lock();
    return param && param->hasExpression(index);
}

QString ParameterItem::expressionAt(int index) const
{
    auto param = parameter_.lock();
    if (!param) return {};
    std::optional<String> expression = param->getExpression(index);
    return expression ? QString::fromStdString(*expression) : QString();
}

QString ParameterItem::expressionErrorAt(int index) const
{
    auto param = parameter_.lock();
    if (!param || !param->hasExpression(index)) return {};

    String error;
    switch (param->getValueType())
    {
    case prm::ValueType::Float:
        param->evalFloat(index, error);
        break;
    case prm::ValueType::Int:
        param->evalInt(index, error);
        break;
    case prm::ValueType::String:
        param->evalString(index, error);
        break;
    }
    return QString::fromStdString(error);
}

void ParameterItem::setExpressionAt(int index, const QString& expression)
{
    if (auto param = parameter_.lock()) param->setExpression(expression.toStdString(), index);
}

void ParameterItem::beginEdit()
{
    if (auto param = parameter_.lock()) snapshotBeforeEdit_ = toSerializable(*param);
}

void ParameterItem::commitEdit()
{
    auto param = parameter_.lock();
    if (!param) return;

    ParameterSerializable after = toSerializable(*param);
    if (after == snapshotBeforeEdit_) return;

    nt::nm().undoStack().push(std::make_unique<nt::ChangeParameterCommand>(
        param->getOpId(),
        param->getName(),
        snapshotBeforeEdit_,
        after
    ));
}

void ParameterItem::setMeta(bool enabled, bool hidden)
{
    if (enabled_ == enabled && hidden_ == hidden) return;
    enabled_ = enabled;
    hidden_ = hidden;
    Q_EMIT metaChanged();
}

} // namespace enzo::ui
