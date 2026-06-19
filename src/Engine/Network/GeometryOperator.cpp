#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/CookContext.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Parameter/Template.h"
#include "Engine/UndoRedo/ChangeConnectionCommand.h"
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

std::weak_ptr<nt::GeometryConnection> nt::connectOperators(
    nt::OpId inputOpId,
    unsigned int inputIndex,
    nt::OpId outputOpId,
    unsigned int outputIndex
)
{
    auto& nm = nt::nm();
    auto updateLock = nm.lockUpdates();

    auto& inputOp = nm.getGeoOperator(inputOpId);
    auto& outputOp = nm.getGeoOperator(outputOpId);

    auto newConnection =
        std::make_shared<nt::GeometryConnection>(inputOpId, inputIndex, outputOpId, outputIndex);

    // set output on the upper operator
    inputOp.addOutputConnection(newConnection);

    // set input on the lower operator
    IC();
    outputOp.addInputConnection(newConnection);

    nm.connectionCreated(newConnection);

    auto cmd = std::make_unique<ChangeConnectionCommand>(
        inputOpId,
        inputIndex,
        outputOpId,
        outputIndex,
        ChangeConnectionCommand::Action::Connect
    );
    nm.undoStack().push(std::move(cmd));

    return newConnection;
}

nt::GeometryOperator::GeometryOperator(nt::OpId opId, op::OpInfo opInfo)
    : opId_{opId}, opInfo_{opInfo}, opDef_(opInfo.ctorFunc(&nt::nm(), opInfo))
{

    initParameters();
}

void nt::GeometryOperator::initParameters()
{
    // Extract parameters from groups
    std::function<void(const prm::Template&)> visit = [&](const prm::Template& templateEntry) {
        if (templateEntry.getType() == prm::Type::GROUP)
        {
            for (const prm::Template& child : templateEntry.getChildren())
                visit(child);
            return;
        }

        auto parameter = std::make_shared<prm::NodeParameter>(templateEntry, opId_);
        parameter->valueChanged.connect([this, name = templateEntry.getName()]() {
            onParameterChanged(name);
        });
        IC(parameter);
        parameters_.push_back(parameter);
    };

    for (const prm::Template& templateEntry : opInfo_.templates)
        visit(templateEntry);
}

void nt::GeometryOperator::dirtyNode(bool dirtyDescendents)
{
    std::cout << "Dirtying op: " << opId_ << "\n";
    dirty_ = true;
    nodeDirtied(opId_, dirtyDescendents);
}

void nt::GeometryOperator::onParameterChanged(const std::string& parmName)
{
    parameterChanged(parmName);
    dirtyNode();
}

bool nt::GeometryOperator::isDirty() { return dirty_; }

void nt::GeometryOperator::cookOp(op::CookContext context)
{
    std::cout << "Cooking op: " << opId_ << "\n";
    if (dirty_)
    {
        opDef_->cookOp(context);
        dirty_ = false;
    }
}

std::shared_ptr<const NodePacket> nt::GeometryOperator::getOutputPacket(unsigned outputIndex) const
{
    return opDef_->getOutputPacket(outputIndex);
}

void nt::GeometryOperator::addInputConnection(std::shared_ptr<nt::GeometryConnection> newConnection)
{
    IC();
    // delete previous input
    std::shared_ptr<nt::GeometryConnection> previousConection = nullptr;
    IC();
    for (auto it = inputConnections_.begin(); it != inputConnections_.end(); ++it)
    {
        if ((*it)->getOutputIndex() == newConnection->getOutputIndex())
        {
            previousConection = *it;
            break;
        }
    }
    if (previousConection)
    {
        previousConection->remove();
    }

    // add new newConnection
    inputConnections_.push_back(newConnection);

    dirtyNode();
}

void nt::GeometryOperator::addOutputConnection(std::shared_ptr<nt::GeometryConnection> connection)
{
    std::cout << "Output connection added\nConnecting ops " << connection->getInputOpId() << " -> "
              << connection->getOutputOpId() << "\n";
    std::cout << "Connecting index " << connection->getInputIndex() << " -> "
              << connection->getOutputIndex() << "\n";
    outputConnections_.push_back(connection);
    std::cout << "size: " << outputConnections_.size() << "\n";
}

void nt::GeometryOperator::removeInputConnection(unsigned int inputIndex)
{
    for (auto it = inputConnections_.begin(); it != inputConnections_.end(); ++it)
    {
        if ((*it)->getOutputIndex() == inputIndex)
        {
            inputConnections_.erase(it);
            dirtyNode();
            std::cerr << "removing input connection\n";
            return;
        }
    }
    std::cerr << "Couldn't remove input connection: " << inputIndex << "\n";
    IC(inputIndex, opId_, inputConnections_.size());
}

void nt::GeometryOperator::removeOutputConnection(const nt::GeometryConnection* connectionPtr)
{
    IC();
    IC(outputConnections_.size());
    for (auto it = outputConnections_.begin(); it != outputConnections_.end(); ++it)
    {
        const nt::GeometryConnection* otherConnectionPtr = (*it).get();
        IC(*connectionPtr);
        if (*connectionPtr == *otherConnectionPtr)
        {
            outputConnections_.erase(it);
            dirtyNode();
            std::cout << "removing output connection\n";
            IC(outputConnections_.size());
            return;
        }
    }
    std::cerr << "-------\nERROR: Couldn't remove output connection\nFailed to find: "
              << *connectionPtr << " in outputConnections size: " << outputConnections_.size()
              << "\n-------\n";
}

std::weak_ptr<prm::NodeParameter> nt::GeometryOperator::getParameter(std::string_view parameterName)
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

bool nt::GeometryOperator::isComparisonTrue(const std::string& conditionText)
{
    const std::optional<ParameterComparison> comparison = parseParameterComparison(conditionText);
    if (!comparison) return false;

    // The controlling parameter the comparison reads from.
    auto controller = getParameter(comparison->controller);
    if (controller.expired()) return false;

    return comparison->isMet(controller.lock()->evalInt());
}

bool nt::GeometryOperator::isParameterEnabled(std::string_view parmName)
{
    // An unknown parameter is treated as enabled.
    auto parameter = getParameter(parmName);
    if (parameter.expired()) return true;

    // Disabled only while its disable comparison is true.
    return !isComparisonTrue(parameter.lock()->getTemplate().getDisableCondition());
}

bool nt::GeometryOperator::isParameterHidden(std::string_view parmName)
{
    // An unknown parameter is treated as shown.
    auto parameter = getParameter(parmName);
    if (parameter.expired()) return false;

    // Hidden only while its hide comparison is true.
    return isComparisonTrue(parameter.lock()->getTemplate().getHideCondition());
}

std::vector<std::weak_ptr<nt::GeometryConnection>> nt::GeometryOperator::getInputConnections() const
{
    return {inputConnections_.begin(), inputConnections_.end()};
}
std::vector<std::weak_ptr<prm::NodeParameter>> nt::GeometryOperator::getParameters()
{
    return {parameters_.begin(), parameters_.end()};
}

const std::vector<prm::Template>& nt::GeometryOperator::getTemplates() const
{
    return opInfo_.templates;
}

std::string nt::GeometryOperator::getName()
{
    // synthesize a placeholder runtime name from the type name and op id
    return opInfo_.internalName + "_" + std::to_string(opId_);
}

std::vector<std::weak_ptr<nt::GeometryConnection>>
nt::GeometryOperator::getOutputConnections() const
{
    return {outputConnections_.begin(), outputConnections_.end()};
}

const op::OpInfo& nt::GeometryOperator::getType() const { return opInfo_; }

std::weak_ptr<nt::GeometryConnection> nt::GeometryOperator::getInputConnection(size_t index) const
{
    for (auto it = inputConnections_.begin(); it != inputConnections_.end(); ++it)
    {
        if ((*it)->getOutputIndex() == index)
        {
            return *it;
        }
    }
    return {};
}

// std::optional<nt::OpId> nt::GeometryOperator::getInput(unsigned int inputNumber) const
// {
//     if(inputNumber>=maxInputs_)
//     {
//         return std::nullopt;
//     }
//     return inputIds_.at(inputNumber);
// }

// std::optional<nt::OpId> nt::GeometryOperator::getOutput(unsigned int outputNumber) const
// {
//     if(outputNumber>=maxOutputs_)
//     {
//         return std::nullopt;
//     }
//     return outputIds_.at(outputNumber);
// }

unsigned int nt::GeometryOperator::getMaxInputs() const { return opDef_->getMaxInputs(); }
unsigned int nt::GeometryOperator::getMaxOutputs() const { return opDef_->getMaxOutputs(); }
unsigned int nt::GeometryOperator::getMinInputs() const { return opDef_->getMinInputs(); }

} // namespace enzo
