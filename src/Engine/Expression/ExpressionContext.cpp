#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NetworkPath.h"
#include "Engine/Parameter/NodeParameter.h"

namespace enzo::expr {

namespace {
template <typename Value>
Value evalParameter(prm::NodeParameter& parameter, unsigned int index)
{
    if constexpr (std::is_same_v<Value, floatT>) return parameter.evalFloat(index);
    else if constexpr (std::is_same_v<Value, intT>) return parameter.evalInt(index);
    else return parameter.evalString(index);
}
} // namespace

template <typename Value>
std::optional<Value> ExpressionContext::readParameter(const String& path, unsigned int index) const
{
    const size_t valueType = ParameterValue(std::in_place_type<Value>).index();
    const ParameterRead read{path, index, valueType};

    std::lock_guard lock(readMutex_);

    auto stored = parameterValues_.find(read);
    if (stored != parameterValues_.end()) return std::get<Value>(stored->second);

    auto parameter = nt::nm().findParameter(NetworkPath(path), currentNode_).lock();
    if (!parameter) return std::nullopt;

    // Records the parameter's node as a dependency so the expression recooks when
    // that node changes.
    expressionDependencies_.push_back(nt::Unit{parameter->getNodeId()});

    const Value value = evalParameter<Value>(*parameter, index);
    parameterValues_.emplace(read, value);
    return value;
}

template std::optional<floatT>
ExpressionContext::readParameter<floatT>(const String&, unsigned int) const;
template std::optional<intT>
ExpressionContext::readParameter<intT>(const String&, unsigned int) const;
template std::optional<String>
ExpressionContext::readParameter<String>(const String&, unsigned int) const;

} // namespace enzo::expr
