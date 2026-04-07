#include "Serializer.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/OperatorTable.h"
#include "Engine/UndoRedo/UndoDisabler.h"
#include "Engine/Parameter/Parameter.h"
#include "NetworkSerializable.h"
#include "cereal/details/helpers.hpp"
#include <iostream>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <fstream>
#include <unordered_map>
#include <variant>

namespace enzo::nt
{

void Serializer::save(NetworkManager& networkManager, std::string filePath)
{
    std::cout << "serializing\n";
    std::ofstream file(filePath);
    cereal::JSONOutputArchive save(file);

    NetworkSerializable networkModel;

    auto ops = networkManager.operators();

    // Build OpId -> index mapping and serialize nodes
    std::unordered_map<nt::OpId, unsigned int> opIdToIndex;
    unsigned int index = 0;
    networkModel.nodes.reserve(ops.size());

    for(auto [opId, op] : ops)
    {
        opIdToIndex[opId] = index++;

        OperatorSerializable opModel;
        opModel.typeName = op.getTypeName();
        opModel.posX = op.getPosition().x();
        opModel.posY = op.getPosition().y();

        for(auto weakPrm : op.getParameters())
        {
            if(auto prm = weakPrm.lock())
            {
                ParameterSerializable prmModel;
                prmModel.name = prm->getName();
                std::visit([&prmModel](const auto& v) {
                    using T = typename std::decay_t<decltype(v)>::value_type;
                    if constexpr (std::is_same_v<T, bt::floatT>)
                        prmModel.floatValues = v;
                    else if constexpr (std::is_same_v<T, bt::intT>)
                        prmModel.intValues = v;
                    else
                        prmModel.stringValues = v;
                }, prm->getValues());
                opModel.parameters.push_back(prmModel);
            }
        }

        networkModel.nodes.push_back(opModel);
    }

    // Serialize connections (collect from output side to avoid duplicates)
    for(auto [opId, op] : ops)
    {
        for(auto weakConn : op.getOutputConnections())
        {
            if(auto conn = weakConn.lock())
            {
                ConnectionSerializable connModel;
                connModel.inputNodeIndex = opIdToIndex[conn->getInputOpId()];
                connModel.inputSocketIndex = conn->getInputIndex();
                connModel.outputNodeIndex = opIdToIndex[conn->getOutputOpId()];
                connModel.outputSocketIndex = conn->getOutputIndex();
                networkModel.connections.push_back(connModel);
            }
        }
    }

    save( CEREAL_NVP(networkModel) );
}

void Serializer::load(NetworkManager& networkManager, std::string filePath)
{
    networkManager.clear();
    UndoDisabler blockAll;

    std::ifstream file(filePath);
    cereal::JSONInputArchive load(file);

    NetworkSerializable network;
    load(network);

    // Create operators and track their new OpIds by index
    std::vector<nt::OpId> opIds;
    opIds.reserve(network.nodes.size());

    for(const OperatorSerializable& node : network.nodes)
    {
        std::optional<op::OpInfo> opInfo = op::OperatorTable::getOpInfo(node.typeName);
        nt::OpId id = nm().createOperator(opInfo.value(), {node.posX, node.posY});
        opIds.push_back(id);

        auto& op = networkManager.getGeoOperator(id);
        for(const ParameterSerializable& prmModel : node.parameters)
        {
            auto weakPrm = op.getParameter(prmModel.name);
            if(auto prm = weakPrm.lock())
            {
                switch(prm->getType())
                {
                    case prm::Type::FLOAT:
                    case prm::Type::XYZ:
                        if(!prmModel.floatValues.empty())
                            prm->setValues(prmModel.floatValues);
                        break;
                    case prm::Type::INT:
                    case prm::Type::BOOL:
                    case prm::Type::TOGGLE:
                        if(!prmModel.intValues.empty())
                            prm->setValues(prmModel.intValues);
                        break;
                    case prm::Type::STRING:
                        if(!prmModel.stringValues.empty())
                            prm->setValues(prmModel.stringValues);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // Recreate connections
    for(const ConnectionSerializable& conn : network.connections)
    {
        connectOperators(
            opIds[conn.inputNodeIndex], conn.inputSocketIndex,
            opIds[conn.outputNodeIndex], conn.outputSocketIndex
        );
    }
}

}
