#include "Engine/Network/Node.h"
#include "Engine/Network/CookContext.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Primitives/Primitive.h"
#include "icecream.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace enzo {

namespace {

// A parsed comparison such as "applyscale == 0", read from a disable or hide
// condition string.
struct ParameterComparison
{
    std::string controller;
    std::string comparator;
    intT target;

    // Whether the controller's value satisfies the comparison.
    bool isMet(intT value) const
    {
        const bool isEqual = value == target;
        if (comparator == "!=") return !isEqual;
        return isEqual;
    }
};

// Reads a comparison string, empty when it is blank or malformed.
std::optional<ParameterComparison> parseParameterComparison(const std::string& text)
{
    std::istringstream stream(text);
    ParameterComparison comparison;
    if (stream >> comparison.controller >> comparison.comparator >> comparison.target)
        return comparison;
    return std::nullopt;
}

} // namespace

nt::Node::Node(nt::NodeId nodeId, const nt::NodeType& nodeType)
    : nodeId_{nodeId}, nodeType_{nodeType},
      path_{"/" + nodeType.internalName + "_" + std::to_string(nodeId)}
{
    // Start every slot on an empty packet so consumers never see null.
    outputPackets_.resize(getMaxOutputs());
    for (auto& slot : outputPackets_)
        slot = std::make_shared<const NodePacket>();

    initParameters();
}

void nt::Node::initParameters()
{
    // Extract parameters from groups
    std::function<void(const prm::Template&)> visit = [&](const prm::Template& templateEntry) {
        if (templateEntry.getType() == prm::Type::GROUP)
        {
            for (const prm::Template& child : templateEntry.getChildren())
                visit(child);
            return;
        }

        auto parameter = std::make_shared<prm::NodeParameter>(templateEntry, nodeId_);
        parameter->valueChanged.connect([this, name = templateEntry.getName()]() {
            onParameterChanged(name);
        });
        IC(parameter);
        parameters_.push_back(parameter);
    };

    for (const prm::Template& templateEntry : nodeType_.templates)
        visit(templateEntry);
}

void nt::Node::dirtyNode(bool dirtyDescendents)
{
    std::cout << "Dirtying node: " << nodeId_ << "\n";
    dirty_ = true;
    nodeDirtied(nodeId_, dirtyDescendents);
}

void nt::Node::onParameterChanged(const std::string& parmName)
{
    parameterChanged(parmName);
    dirtyNode();
}

bool nt::Node::isDirty() { return dirty_; }

void nt::Node::cook(nt::CookContext& context)
{
    std::cout << "Cooking node: " << nodeId_ << "\n";
    if (!dirty_) return;

    // The implementation lives only as long as the cook, so nothing it holds
    // can survive into the next one.
    const std::unique_ptr<nt::NodeImpl> implementation(nodeType_.ctorFunc(*this, context));
    implementation->cook();
    dirty_ = false;
}

std::shared_ptr<const NodePacket> nt::Node::getOutputPacket(unsigned outputIndex) const
{
    if (outputIndex >= getMaxOutputs())
    {
        throw std::runtime_error("Cannot get output packet at an index past the node's outputs");
    }

    return outputPackets_.at(outputIndex);
}

void nt::Node::setOutputPacket(unsigned int outputIndex, NodePacket packet)
{
    if (outputIndex >= getMaxOutputs())
    {
        throw std::runtime_error("Cannot set output packet at an index past the node's outputs");
    }

    // Defragment every primitive so downstream consumers see contiguous offsets.
    for (auto& prim : packet.getPrimitives())
    {
        if (prim) prim->defragment();
    }

    outputPackets_[outputIndex] = std::make_shared<const NodePacket>(std::move(packet));
}

bool nt::Node::outputRequested(unsigned int outputIndex) const
{
    // TODO: implement
    return true;
}

std::weak_ptr<prm::NodeParameter> nt::Node::getParameter(std::string_view parameterName)
{
    for (auto parm : parameters_)
    {
        if (parm->getName() == parameterName)
        {
            return parm;
        }
    }
    return std::weak_ptr<prm::NodeParameter>();
}

bool nt::Node::isComparisonTrue(const std::string& conditionText)
{
    const std::optional<ParameterComparison> comparison = parseParameterComparison(conditionText);
    if (!comparison) return false;

    // The controlling parameter the comparison reads from.
    auto controller = getParameter(comparison->controller);
    if (controller.expired()) return false;

    return comparison->isMet(controller.lock()->evalInt());
}

bool nt::Node::isParameterEnabled(std::string_view parmName)
{
    // An unknown parameter is treated as enabled.
    auto parameter = getParameter(parmName);
    if (parameter.expired()) return true;

    // Disabled only while its disable comparison is true.
    return !isComparisonTrue(parameter.lock()->getTemplate().getDisableCondition());
}

bool nt::Node::isParameterHidden(std::string_view parmName)
{
    // An unknown parameter is treated as shown.
    auto parameter = getParameter(parmName);
    if (parameter.expired()) return false;

    // Hidden only while its hide comparison is true.
    return isComparisonTrue(parameter.lock()->getTemplate().getHideCondition());
}

std::vector<std::weak_ptr<prm::NodeParameter>> nt::Node::getParameters()
{
    return {parameters_.begin(), parameters_.end()};
}

const std::vector<prm::Template>& nt::Node::getTemplates() const { return nodeType_.templates; }

std::string nt::Node::getName() const { return path_.getName(); }

const nt::NodeType& nt::Node::getType() const { return nodeType_; }

// std::optional<nt::NodeId> nt::Node::getInput(unsigned int inputNumber) const
// {
//     if(inputNumber>=maxInputs_)
//     {
//         return std::nullopt;
//     }
//     return inputIds_.at(inputNumber);
// }

// std::optional<nt::NodeId> nt::Node::getOutput(unsigned int outputNumber) const
// {
//     if(outputNumber>=maxOutputs_)
//     {
//         return std::nullopt;
//     }
//     return outputIds_.at(outputNumber);
// }

unsigned int nt::Node::getMaxInputs() const { return nodeType_.maxInputs; }
unsigned int nt::Node::getMaxOutputs() const { return nodeType_.maxOutputs; }
unsigned int nt::Node::getMinInputs() const { return nodeType_.minInputs; }

} // namespace enzo
