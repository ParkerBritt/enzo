#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/Geometry.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Operator/OpInfo.h"
#include "Engine/Types.h"
#include <iostream>
#include <memory>
#include <stack>
#include <algorithm>
#include <stdexcept>
#include <string>
#include "icecream.hpp"

enzo::nt::OpId enzo::nt::NetworkManager::addOperator(op::OpInfo opInfo)
{

    maxOpId_++;
    std::unique_ptr<GeometryOperator> newOp = std::make_unique<GeometryOperator>(maxOpId_, opInfo);
    newOp->nodeDirtied.connect(
        [this](nt::OpId opId, bool dirtyDependents)
        {
            if(dirtyDependents)
            {
                std::vector<OpId> dependentIds = getDependentsGraph(opId);
                for(OpId dependentId : dependentIds)
                {
                    // dirty node
                    enzo::nt::GeometryOperator& dependentOp = getGeoOperator(dependentId);
                    std::cout << "Manager dirtying id: " << dependentId << "\n";
                    dependentOp.dirtyNode(false);

                    // cook display op
                    if(getDisplayOp().has_value() && getDisplayOp().value()==dependentId)
                    {
                        cookOp(dependentId);
                        displayGeoChanged(dependentOp.getOutputGeo(0));
                    }
                }
            }

        });
    gopStore_.emplace(maxOpId_, std::move(newOp));

    return maxOpId_;
}


enzo::nt::NetworkManager& enzo::nt::NetworkManager::getInstance()
{
    static enzo::nt::NetworkManager instance;
    return instance;
}

enzo::nt::GeometryOperator& enzo::nt::NetworkManager::getGeoOperator(nt::OpId opId)
{
    auto it = gopStore_.find(opId);
    if(it == gopStore_.end())
    {
        throw std::out_of_range("OpId: " + std::to_string(opId) + " > max opId: " + std::to_string(maxOpId_) + "\n");
    }
    return *it->second;
}

bool enzo::nt::NetworkManager::isValidOp(nt::OpId opId)
{
    auto it = gopStore_.find(opId);
    if( it == gopStore_.end() || it->second==nullptr )
    {
        return false;
    }
    return true;
}

void enzo::nt::NetworkManager::setDisplayOp(OpId opId)
{
    displayOp_=opId;
    
    cookOp(opId);

    enzo::nt::GeometryOperator& displayOp = getGeoOperator(opId);
    displayGeoChanged(displayOp.getOutputGeo(0));
    displayNodeChanged(opId);
}

void enzo::nt::NetworkManager::setSelectedNode(OpId opId, bool selected, bool add)
{
    if(add)
    {
        auto idIter = std::find(selectedNodes_.begin(), selectedNodes_.end(), opId);
        if(selected)
        {
            // skip if value is already in selected nodes
            if(idIter!=selectedNodes_.end()) return;
            selectedNodes_.push_back(opId);
            cookOp(opId);
        }
        else
        {
            // skip if value is not in selected nodes
            if(idIter==selectedNodes_.end()) return;
            selectedNodes_.erase(idIter);
        }
    }
    else
    {
        if(selected)
        {
            selectedNodes_.clear();
            selectedNodes_.push_back(opId);
            cookOp(opId);
        }
        else
        {
            selectedNodes_.clear();
        }
    }
    selectedNodesChanged(selectedNodes_);

}

const std::vector<enzo::nt::OpId>& enzo::nt::NetworkManager::getSelectedNodes()
{
    return selectedNodes_;
}

void enzo::nt::NetworkManager::cookOp(enzo::nt::OpId opId)
{
    std::vector<enzo::nt::OpId> dependencyGraph = getDependencyGraph(opId);

    for(enzo::nt::OpId dependencyOpId : dependencyGraph)
    {
        enzo::nt::GeometryOperator& op = getGeoOperator(dependencyOpId);
        if(op.isDirty())
        {
            enzo::op::Context context(dependencyOpId, enzo::nt::nm());
            op.cookOp(context);
        }
    }
}

std::vector<enzo::nt::OpId> enzo::nt::NetworkManager::getDependencyGraph(enzo::nt::OpId opId)
{
    std::stack<enzo::nt::OpId> traversalBuffer;
    std::vector<enzo::nt::OpId> dependencyGraph;
    traversalBuffer.push(opId);
    dependencyGraph.push_back(opId);

    while(traversalBuffer.size()!=0)
    {
        enzo::nt::OpId currentOp = traversalBuffer.top();
        traversalBuffer.pop();
        auto inputConnections = getGeoOperator(currentOp).getInputConnections();
        for(auto connection : inputConnections)
        {
            if(auto connectionPtr = connection.lock())
            {
                traversalBuffer.push(connectionPtr->getInputOpId());
                dependencyGraph.push_back(connectionPtr->getInputOpId());
            }
            else { throw std::runtime_error("Connection weak ptr invalid"); }
        }
    }

    std::reverse(dependencyGraph.begin(), dependencyGraph.end());
    return dependencyGraph;
}

std::vector<enzo::nt::OpId> enzo::nt::NetworkManager::getDependentsGraph(enzo::nt::OpId opId)
{
    std::stack<enzo::nt::OpId> traversalBuffer;
    std::vector<enzo::nt::OpId> dependencyGraph;
    traversalBuffer.push(opId);
    dependencyGraph.push_back(opId);

    while(traversalBuffer.size()!=0)
    {
        enzo::nt::OpId currentOp = traversalBuffer.top();
        traversalBuffer.pop();
        auto outputConnections = getGeoOperator(currentOp).getOutputConnections();
        for(auto connection : outputConnections)
        {
            if(auto connectionPtr = connection.lock())
            {
                traversalBuffer.push(connectionPtr->getOutputOpId());
                dependencyGraph.push_back(connectionPtr->getOutputOpId());
            }
            else { throw std::runtime_error("Connection weak ptr invalid"); }
        }
    }

    return dependencyGraph;
}

std::optional<enzo::nt::OpId> enzo::nt::NetworkManager::getDisplayOp()
{
    return displayOp_;
}

#ifdef UNIT_TEST
void enzo::nt::NetworkManager::_reset()
{
    std::cout << "resetting network manager\n";

    gopStore_.clear();
    maxOpId_=0;
    displayOp_.reset();
}
#endif

// std::unordered_map<enzo::nt::OpId, std::unique_ptr<enzo::nt::GeometryOperator>> enzo::nt::NetworkManager::gopStore_;

