#include "Engine/GeometryAlgorithms/BooleanUtils.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"

namespace {

class Boolean : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Boolean::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packetA = cloneInputPacket(0);
    NodePacket packetB = cloneInputPacket(1);

    // Pick the first mesh primitive from each input.
    std::shared_ptr<geo::Mesh> meshA;
    std::shared_ptr<geo::Mesh> meshB;
    for (auto prim : packetA.getPrimitives())
    {
        if (prim->getType() == geo::PrimType::MESH)
        {
            meshA = std::static_pointer_cast<geo::Mesh>(prim);
            break;
        }
    }
    for (auto prim : packetB.getPrimitives())
    {
        if (prim->getType() == geo::PrimType::MESH)
        {
            meshB = std::static_pointer_cast<geo::Mesh>(prim);
            break;
        }
    }

    NodePacket output;
    if (meshA && meshB)
    {
        const std::string opStr = evalParmString("operation");
        utils::BooleanOp op = utils::BooleanOp::UNION;
        if (opStr == "intersect")
            op = utils::BooleanOp::INTERSECT;
        else if (opStr == "subtract")
            op = utils::BooleanOp::SUBTRACT;

        std::string error;
        std::shared_ptr<geo::Mesh> result = utils::booleanMesh(*meshA, *meshB, op, &error);
        if (!error.empty()) throwError(error);
        output.addPrimitive(result);
    }
    else if (meshA)
    {
        // When only one input has a mesh, pass it through unchanged.
        output.addPrimitive(meshA);
    }

    setOutputPacket(0, output);
}

} // namespace

ENZO_REGISTER_NODE(boolean, Boolean)
