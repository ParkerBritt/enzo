#include "Serializer.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/OperatorTable.h"
#include "NetworkSerializable.h"
#include "cereal/details/helpers.hpp"
#include <iostream>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <fstream>
#include <unordered_map>

namespace enzo::nt
{

void Serializer::save(NetworkManager& networkManager)
{
    std::cout << "serializing\n";
    std::ofstream file("/home/parker/Downloads/test.enz");
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
                unsigned int vecSize = prm->getVectorSize();
                for(unsigned int i = 0; i < vecSize; i++)
                {
                    prmModel.floatValues.push_back(prm->evalFloat(i));
                    prmModel.intValues.push_back(prm->evalInt(i));
                    prmModel.stringValues.push_back(prm->evalString(i));
                }
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

void Serializer::load(NetworkManager& networkManager)
{
    networkManager.clear();

    std::ifstream file("/home/parker/Downloads/test.enz");
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
                unsigned int vecSize = prm->getVectorSize();
                for(unsigned int i = 0; i < vecSize; i++)
                {
                    if(i < prmModel.floatValues.size())
                        prm->setFloat(prmModel.floatValues[i], i);
                    if(i < prmModel.intValues.size())
                        prm->setInt(prmModel.intValues[i], i);
                    if(i < prmModel.stringValues.size())
                        prm->setString(prmModel.stringValues[i], i);
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
