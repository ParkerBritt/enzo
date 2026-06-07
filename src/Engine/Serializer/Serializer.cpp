#include "Engine/Serializer/Serializer.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Serializer/NetworkSerializable.h"
#include "Engine/UndoRedo/UndoDisabler.h"
#include "cereal/details/helpers.hpp"
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <fstream>
#include <iostream>
#include <unordered_map>

namespace enzo::nt {

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

    for (auto [opId, op] : ops)
    {
        opIdToIndex[opId] = index++;

        OperatorSerializable opModel;
        opModel.typeName = op.getType().getName();
        opModel.posX = op.getPosition().x();
        opModel.posY = op.getPosition().y();

        for (auto weakPrm : op.getParameters())
        {
            if (auto prm = weakPrm.lock())
            {
                ParameterSerializable prmModel;
                prmModel.name = prm->getName();
                switch (prm->getValueType())
                {
                case prm::ValueType::Float:
                    prmModel.floatValues = prm->evalFloats();
                    break;
                case prm::ValueType::Int:
                    prmModel.intValues = prm->evalInts();
                    break;
                case prm::ValueType::String:
                    prmModel.stringValues = prm->evalStrings();
                    break;
                }
                opModel.parameters.push_back(prmModel);
            }
        }

        networkModel.nodes.push_back(opModel);
    }

    // Serialize connections (collect from output side to avoid duplicates)
    for (auto [opId, op] : ops)
    {
        for (auto weakConn : op.getOutputConnections())
        {
            if (auto conn = weakConn.lock())
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

    save(CEREAL_NVP(networkModel));
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

    for (const OperatorSerializable& node : network.nodes)
    {
        std::optional<op::OpInfo> opInfo = op::OperatorTable::getOpInfo(node.typeName);
        nt::OpId id = nm().createOperator(opInfo.value(), {node.posX, node.posY});
        opIds.push_back(id);

        auto& op = networkManager.getGeoOperator(id);
        for (const ParameterSerializable& prmModel : node.parameters)
        {
            auto weakPrm = op.getParameter(prmModel.name);
            if (auto prm = weakPrm.lock())
            {
                switch (prm->getValueType())
                {
                case prm::ValueType::Float:
                    if (!prmModel.floatValues.empty()) prm->setValues(prmModel.floatValues);
                    break;
                case prm::ValueType::Int:
                    if (!prmModel.intValues.empty()) prm->setValues(prmModel.intValues);
                    break;
                case prm::ValueType::String:
                    if (!prmModel.stringValues.empty()) prm->setValues(prmModel.stringValues);
                    break;
                }
            }
        }
    }

    // Recreate connections
    for (const ConnectionSerializable& conn : network.connections)
    {
        connectOperators(
            opIds[conn.inputNodeIndex],
            conn.inputSocketIndex,
            opIds[conn.outputNodeIndex],
            conn.outputSocketIndex
        );
    }
}

} // namespace enzo::nt
