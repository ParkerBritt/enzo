#pragma once
#include "Engine/Operator/GeometryConnection.h"
#include "Engine/Operator/OpInfo.h"
#include "Engine/Operator/GeometryOpDef.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Types.h"
#include <optional>
#include <memory>

namespace enzo::nt {
void connectOperators(enzo::nt::OpId inputOpId, unsigned int inputIndex, enzo::nt::OpId outputOpId, unsigned int outputIndex);

class GeometryOperator
{
public:
    GeometryOperator(enzo::nt::OpId opId, op::OpInfo opInfo);

    // disable copying
    GeometryOperator(const GeometryOperator&)            = delete;
    GeometryOperator& operator=(const GeometryOperator&) = delete;

    void cookOp(op::Context context);
    geo::Geometry& getOutputGeo(unsigned outputIndex) const;

    void addInputConnection(std::shared_ptr<nt::GeometryConnection> connection);
    void addOutputConnection(std::shared_ptr<nt::GeometryConnection> connection);
    std::vector<std::weak_ptr<const GeometryConnection>> getInputConnections() const;
    std::vector<std::weak_ptr<const GeometryConnection>> getOutputConnections() const;
    std::optional<const GeometryConnection> getInputConnection(size_t index) const;
    std::vector<std::weak_ptr<prm::Parameter>> getParameters();
    std::weak_ptr<prm::Parameter> getParameter(std::string parameterName);

    std::string getLabel(); // TODO: implement node labels
    std::string getTypeName();

    void dirtyNode(bool dirtyDescendents=true);
    bool isDirty();


    unsigned int getMinInputs() const;
    unsigned int getMaxInputs() const;
    unsigned int getMaxOutputs() const;

    // signals
    boost::signals2::signal<void (nt::OpId opId, bool dirtyDescendents)> nodeDirtied;



private:
    void initParameters();

    // TODO: avoid duplicate connections
    std::vector<std::shared_ptr<nt::GeometryConnection>> inputConnections_;
    std::vector<std::shared_ptr<nt::GeometryConnection>> outputConnections_;
    std::vector<std::shared_ptr<prm::Parameter>> parameters_; 
    std::unique_ptr<enzo::nt::GeometryOpDef> opDef_;
    enzo::nt::OpId opId_;
    enzo::op::OpInfo opInfo_;
    bool dirty_ = true;
};
}
