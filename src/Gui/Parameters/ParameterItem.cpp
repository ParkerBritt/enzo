#include "Gui/Parameters/ParameterItem.h"
#include "Engine/Attribute/Attribute.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/NetworkGraph/NetworkGraph.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Parameter/StyleAccess.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Primitives/Primitive.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include <algorithm>

namespace enzo::ui {

namespace {

/// @brief Whether the type carries a numeric range worth reading.
/// TODO: I'm not sure about this function. Should probably look at removing
bool hasRange(prm::Type type) { return type == prm::Type::FLOAT || type == prm::Type::INT; }

/// @brief Returns a style's options how QML reads them, keyed by option name.
QVariantMap getStyleOptions(const std::any& style)
{
    QVariantMap values;
    for (const std::shared_ptr<prm::Parameter>& option : prm::style::options(style))
    {
        const QString optionName = QString::fromStdString(option->getName());
        switch (option->getValueType())
        {
        case prm::ValueType::Float:
            values[optionName] = static_cast<double>(option->evalFloat());
            break;
        case prm::ValueType::Int:
            values[optionName] = option->getType() == prm::Type::BOOL
                                     ? QVariant(option->evalInt() != 0)
                                     : QVariant(static_cast<qlonglong>(option->evalInt()));
            break;
        case prm::ValueType::String:
            values[optionName] = QString::fromStdString(option->evalString());
            break;
        }
    }
    return values;
}

/// @brief Returns the owners or types a style option names, reading the
/// parameter it points at when it is written as prm(name).
/// @note A option that reads back empty, or names an owner or type that does
/// not exist, gives all of them.
template <typename Parse>
auto getStyleOptionValues(const std::string& optionText, nt::NodeId nodeId, Parse parseValue)
{
    const std::optional<std::string> text = nt::nm().getNode(nodeId).getReferencedValue(optionText);
    if (!text || text->empty()) return parseValue("all");

    try
    {
        return parseValue(*text);
    }
    catch (const std::runtime_error&)
    {
        return parseValue("all");
    }
}

/// @brief Returns the attribute names in a packet the style accepts, sorted alphabetically.
/// @note Private attributes stay out of the list.
QStringList getAttributeNames(
    const NodePacket& packet,
    const std::vector<attr::AttributeOwner>& owners,
    const std::vector<attr::AttributeType>& types
)
{
    QStringList names;
    const auto primitiveCount = static_cast<unsigned int>(packet.size());
    for (unsigned int primitiveIndex = 0; primitiveIndex < primitiveCount; ++primitiveIndex)
    {
        const std::shared_ptr<const geo::Primitive> primitive = packet.getPrimitive(primitiveIndex);
        if (!primitive) continue;

        for (attr::AttributeOwner owner : owners)
        {
            const auto attributeCount =
                static_cast<unsigned int>(primitive->getNumAttributes(owner));
            for (unsigned int attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex)
            {
                const auto attribute = primitive->getAttributeByIndex(owner, attributeIndex).lock();
                if (!attribute || attribute->isPrivate()) continue;

                const bool typeWanted =
                    std::find(types.begin(), types.end(), attribute->getType()) != types.end();
                if (!typeWanted) continue;

                const QString name = QString::fromStdString(attribute->getName());
                if (!names.contains(name)) names.append(name);
            }
        }
    }

    names.sort(Qt::CaseInsensitive);
    return names;
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
    nt::Node& node,
    QObject* parent
)
    : QObject(parent), parameter_(std::move(parameter))
{
    kind_ = QString::fromStdString(prm::toString(prmTemplate.getType()));
    style_ = QString::fromStdString(prm::style::toString(prmTemplate.getStyle()));
    styleOptions_ = getStyleOptions(prmTemplate.getStyle());
    name_ = QString::fromStdString(prmTemplate.getName());
    nodeName_ = QString::fromStdString(node.getName());
    label_ = QString::fromStdString(prmTemplate.getLabel());
    tooltip_ = QString::fromStdString(prmTemplate.getTooltip());
    icon_ = QString::fromStdString(prmTemplate.getIcon());
    vectorSize_ = static_cast<int>(prmTemplate.getSize());
    horizontal_ = prmTemplate.getDirection() == prm::Direction::HORIZONTAL;
    labelHidden_ = prmTemplate.isLabelHidden();

    if (prm::style::holds<prm::style::Attribute>(prmTemplate.getStyle()))
        attributeStyle_ =
            std::any_cast<std::shared_ptr<prm::style::Attribute>>(prmTemplate.getStyle());

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

    // An undo, direct edit, or upstream dependency can all change the value
    // behind QML's back, and the owning node broadcasts every one of them
    // through this signal, so mirror it out as the QML notify.
    const std::string parmName = prmTemplate.getName();
    valueSubscription_ = node.parameterChanged.connect([this, parmName](const std::string& name) {
        if (name == parmName) Q_EMIT valueChanged();
    });
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

void ParameterItem::clearExpressionAt(int index)
{
    if (auto param = parameter_.lock()) param->clearExpression(index);
}

QVariantMap ParameterItem::previewExpressionAt(int index, const QString& expression) const
{
    auto param = parameter_.lock();
    if (!param) return {};

    const String text = expression.toStdString();
    String error;
    QVariant value;
    switch (param->getValueType())
    {
    case prm::ValueType::Float:
        value = static_cast<double>(param->previewFloat(text, error));
        break;
    case prm::ValueType::Int:
        value = static_cast<qlonglong>(param->previewInt(text, error));
        break;
    case prm::ValueType::String:
        value = QString::fromStdString(param->previewString(text, error));
        break;
    }

    QVariantMap result;
    result["value"] = value;
    result["invalid"] = !error.empty();
    return result;
}

QStringList ParameterItem::attributeNames() const
{
    auto param = parameter_.lock();
    if (!param || !attributeStyle_) return {};

    const std::optional<nt::Connection> input =
        nt::nm().graph().getInputConnection(param->getNodeId(), 0);
    if (!input) return {};

    const std::shared_ptr<const NodePacket> packet =
        nt::nm().getNode(input->sourceNode).getOutputPacket(input->sourceOutput);
    if (!packet) return {};

    const nt::NodeId nodeId = param->getNodeId();
    const std::vector<attr::AttributeOwner> owners =
        getStyleOptionValues(attributeStyle_->owners(), nodeId, prm::style::parseOwners);
    const std::vector<attr::AttributeType> types = getStyleOptionValues(
        attributeStyle_->attributeTypes(),
        nodeId,
        prm::style::parseAttributeTypes
    );

    return getAttributeNames(*packet, owners, types);
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

    nt::nm().undoStack().push(
        std::make_unique<nt::ChangeParameterCommand>(
            param->getNodeId(),
            param->getName(),
            snapshotBeforeEdit_,
            after
        )
    );
}

void ParameterItem::setMeta(bool enabled, bool hidden)
{
    if (enabled_ == enabled && hidden_ == hidden) return;
    enabled_ = enabled;
    hidden_ = hidden;
    Q_EMIT metaChanged();
}

} // namespace enzo::ui
