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

namespace enzo::nt
{

void Serializer::save(NetworkManager& networkManager)
{
    std::cout << "serializing\n";
    // cereal::JSONOutputArchive output(std::cout); // stream to cout
    std::ofstream file("/home/parker/Downloads/test.enz");
    cereal::JSONOutputArchive save(file);
    bool arr[] = {true, false};
    std::vector<int> vec = {1, 2, 3, 4, 5};

    // Create serializable network model
    NetworkSerializable networkModel;

    // Get operators in network
    auto ops = networkManager.operators();

    // Resize model nodes vector for performance
    networkModel.nodes.reserve(ops.size());

    // Add all nodes to network model
    for(auto [opId, op] : ops)
    {
        OperatorSerializable opModel;
        opModel.typeName = op.getTypeName();
        std::cout << "iterating " << opModel.typeName << "\n";
        networkModel.nodes.push_back(opModel);
    }


    save( CEREAL_NVP(networkModel) );
}

void Serializer::load(NetworkManager& networkManager)
{
    std::ifstream file("/home/parker/Downloads/test.enz");
    cereal::JSONInputArchive load(file);

    NetworkSerializable network;
    load(network);

    for( OperatorSerializable node : network.nodes) 
    {
        std::optional<op::OpInfo> opInfo = op::OperatorTable::getOpInfo( node.typeName);
        nm().addOperator(opInfo.value());
    }

    std::cout << "node name:" << network.nodes[0].typeName << "\n";
}

}
