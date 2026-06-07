#include "Engine/Serializer/Serializer.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Serializer/NetworkSerializable.h"
#include "Engine/Serializer/ParameterSerializable.h"
#include "Engine/UndoRedo/UndoDisabler.h"
#include "cereal/details/helpers.hpp"
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <fstream>
#include <iostream>
#include <unordered_map>

ParameterSerializable toSerializable(enzo::prm::Parameter& parameter)
{
    ParameterSerializable model;
    model.name = parameter.getName();

    if (parameter.getTemplate().isMultiParm())
    {
        const unsigned int instanceCount = parameter.getInstanceCount();
        model.instances.reserve(instanceCount);
        for (unsigned int instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
        {
            std::vector<ParameterSerializable> fields;
            for (const auto& field : parameter.getInstance(instanceIndex))
                fields.push_back(toSerializable(*field));
            model.instances.push_back(std::move(fields));
        }
        return model;
    }

    switch (parameter.getValueType())
    {
    case enzo::prm::ValueType::Float:
        model.floatValues = parameter.evalFloats();
        break;
    case enzo::prm::ValueType::Int:
        model.intValues = parameter.evalInts();
        break;
    case enzo::prm::ValueType::String:
        model.stringValues = parameter.evalStrings();
        break;
    }
    return model;
}

void applySerializable(enzo::prm::Parameter& parameter, const ParameterSerializable& model)
{
    if (parameter.getTemplate().isMultiParm())
    {
        // The parameter starts at its template default count. Reconcile to the
        // saved count before writing each instance.
        while (parameter.getInstanceCount() < model.instances.size())
            parameter.addInstance();
        while (parameter.getInstanceCount() > model.instances.size())
            parameter.removeInstance(parameter.getInstanceCount() - 1);

        for (unsigned int instanceIndex = 0; instanceIndex < model.instances.size();
             ++instanceIndex)
        {
            for (const ParameterSerializable& fieldModel : model.instances[instanceIndex])
            {
                auto field = parameter.getInstanceField(instanceIndex, fieldModel.name);
                if (field) applySerializable(*field, fieldModel);
            }
        }
        return;
    }

    switch (parameter.getValueType())
    {
    case enzo::prm::ValueType::Float:
        if (!model.floatValues.empty()) parameter.setValues(model.floatValues);
        break;
    case enzo::prm::ValueType::Int:
        if (!model.intValues.empty()) parameter.setValues(model.intValues);
        break;
    case enzo::prm::ValueType::String:
        if (!model.stringValues.empty()) parameter.setValues(model.stringValues);
        break;
    }
}

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
            if (auto prm = weakPrm.lock()) opModel.parameters.push_back(toSerializable(*prm));
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
            if (auto prm = weakPrm.lock()) applySerializable(*prm, prmModel);
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
