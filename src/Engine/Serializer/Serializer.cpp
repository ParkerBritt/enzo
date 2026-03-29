#include "Serializer.h"
#include "Engine/Network/NetworkManager.h"
#include "NetworkSerializable.h"
#include "cereal/details/helpers.hpp"
#include <iostream>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <fstream>

namespace enzo::nt
{

Serializer::Serializer(NetworkManager& networkManager)
{
    {
        std::cout << "serializing\n";
        // cereal::JSONOutputArchive output(std::cout); // stream to cout
        std::ofstream file("/home/parker/Downloads/test.enz");
        cereal::JSONOutputArchive save(file);
        bool arr[] = {true, false};
        std::vector<int> vec = {1, 2, 3, 4, 5};

        NetworkSerializable network;

        OperatorSerializable op;
        op.typeName = "test";

        network.nodes.push_back(op);

        save( CEREAL_NVP(network) );
    }


    {
        std::ifstream file("/home/parker/Downloads/test.enz");
        cereal::JSONInputArchive load(file);

        NetworkSerializable network;
        load(network);

        std::cout << "node name:" << network.nodes[0].typeName << "\n";
    }
};

}
