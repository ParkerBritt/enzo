#include "Engine/Operator/GeometryOperator.h"
#include <functional>
#include <memory>
#include "Engine/Network/NetworkManager.h"
#include <optional>
#include "Engine/Operator/Context.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/Template.h"
#include <iostream>
#include <stdexcept>
#include "icecream.hpp"

using namespace enzo;

std::weak_ptr<nt::GeometryConnection> enzo::nt::connectOperators(enzo::nt::OpId inputOpId, unsigned int inputIndex, enzo::nt::OpId outputOpId, unsigned int outputIndex)
{
    auto& nm = nt::nm();
    auto updateLock = nm.lockUpdates();

    auto& inputOp = nm.getGeoOperator(inputOpId);
    auto& outputOp = nm.getGeoOperator(outputOpId);

    auto newConnection = std::make_shared<nt::GeometryConnection>(inputOpId, inputIndex, outputOpId, outputIndex); 

    // set output on the upper operator
    inputOp.addOutputConnection(newConnection);

    // set input on the lower operator
    IC();
    outputOp.addInputConnection(newConnection);

    nm.connectionCreated(newConnection);

    return newConnection;
}

nt::GeometryOperator::GeometryOperator(enzo::nt::OpId opId, op::OpInfo opInfo)
: opId_{opId}, opInfo_{opInfo}, opDef_(opInfo.ctorFunc(&nt::nm(), opInfo))
{

    initParameters();
}

void nt::GeometryOperator::initParameters()
{
    for(const prm::Template* t = opInfo_.templates; t->getType()!=prm::Type::LIST_TERMINATOR; ++t)
    {
        // create parameter
        auto parameter = std::make_shared<prm::Parameter>(*t, opId_);
        parameter->valueChanged.connect([this](){dirtyNode();});
        IC(parameter);

        parameters_.push_back(parameter);
    }

}

void enzo::nt::GeometryOperator::dirtyNode(bool dirtyDescendents)
{
    std::cout << "Dirtying op: " << opId_ << "\n";
    dirty_=true;
    nodeDirtied(opId_, dirtyDescendents);
}

bool enzo::nt::GeometryOperator::isDirty()
{
    return dirty_;
}


void enzo::nt::GeometryOperator::cookOp(op::Context context)
{
    std::cout << "Cooking op: " << opId_ << "\n";
    if(dirty_)
    {
        opDef_->cookOp(context);
        dirty_=false;
    }
}

enzo::NodePacket& enzo::nt::GeometryOperator::getOutputPacket(unsigned outputIndex) const
{
    return opDef_->getOutputPacket(outputIndex);
}

void nt::GeometryOperator::addInputConnection(std::shared_ptr<nt::GeometryConnection> newConnection)
{
    IC();
    // delete previous input
    std::shared_ptr<nt::GeometryConnection> previousConection = nullptr;
    IC();
    for(auto it=inputConnections_.begin(); it!=inputConnections_.end(); ++it)
    {
        if((*it)->getOutputIndex()==newConnection->getOutputIndex())
        {
            previousConection = *it;
            break;
        }
    }
    if(previousConection)
    {
        previousConection->remove();
    }

    // add new newConnection
    inputConnections_.push_back(newConnection); 

    dirtyNode();
}

void nt::GeometryOperator::addOutputConnection(std::shared_ptr<nt::GeometryConnection> connection)
{
    std::cout << "Output connection added\nConnecting ops " << connection->getInputOpId() << " -> " << connection->getOutputOpId() << "\n";
    std::cout << "Connecting index " << connection->getInputIndex() << " -> " << connection->getOutputIndex() << "\n";
    outputConnections_.push_back(connection); 
    std::cout << "size: " << outputConnections_.size() << "\n";
}

void nt::GeometryOperator::removeInputConnection(unsigned int inputIndex)
{
    for(auto it=inputConnections_.begin(); it!=inputConnections_.end(); ++it)
    {
        if((*it)->getOutputIndex() == inputIndex)
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
    for(auto it=outputConnections_.begin(); it!=outputConnections_.end(); ++it)
    {
        const nt::GeometryConnection* otherConnectionPtr = (*it).get();
        IC(*connectionPtr);
        if(*connectionPtr == *otherConnectionPtr)
        {
            outputConnections_.erase(it);
            dirtyNode();
            std::cout << "removing output connection\n";
            IC(outputConnections_.size());
            return;
        }
    }
    std::cerr << "-------\nERROR: Couldn't remove output connection\nFailed to find: " << *connectionPtr << " in outputConnections size: " << outputConnections_.size() << "\n-------\n";

}


std::weak_ptr<prm::Parameter> nt::GeometryOperator::getParameter(std::string parameterName)
{
    for(auto parm : parameters_)
    {
        if(parm->getName()==parameterName)
        {
            return parm;
        }
    }
    return std::weak_ptr<prm::Parameter>();

}

std::vector<std::weak_ptr<const nt::GeometryConnection>> nt::GeometryOperator::getInputConnections() const
{
    return {inputConnections_.begin(), inputConnections_.end()};
}
std::vector<std::weak_ptr<prm::Parameter>> nt::GeometryOperator::getParameters()
{
    return {parameters_.begin(), parameters_.end()};
}

std::string nt::GeometryOperator::getLabel()
{
    return opInfo_.displayName; 
}


std::vector<std::weak_ptr<const nt::GeometryConnection>> nt::GeometryOperator::getOutputConnections() const
{
    return {outputConnections_.begin(), outputConnections_.end()};
}

std::string nt::GeometryOperator::getTypeName()
{
    return opInfo_.internalName;
}


std::optional<std::reference_wrapper<const nt::GeometryConnection>> nt::GeometryOperator::getInputConnection(size_t index) const
{
    for(auto it=inputConnections_.begin(); it!=inputConnections_.end(); ++it)
    {
        if((*it)->getOutputIndex()==index)
        {
            return std::cref(**it);
        }
    }
    return std::nullopt;
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


unsigned int nt::GeometryOperator::getMaxInputs() const
{
    return opDef_->getMaxInputs();
}
unsigned int nt::GeometryOperator::getMaxOutputs() const
{
    return opDef_->getMaxOutputs();
}
unsigned int nt::GeometryOperator::getMinInputs() const
{
    return opDef_->getMinInputs();
}
