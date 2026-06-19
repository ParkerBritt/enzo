#include "OpDefs/GopBoolean.h"
#include "Engine/GeometryAlgorithms/BooleanUtils.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Mesh.h"

GopBoolean::GopBoolean(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopBoolean::cookOp(enzo::op::CookContext context)
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packetA = context.cloneInputPacket(0);
    NodePacket packetB = context.cloneInputPacket(1);

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
        const std::string opStr = context.evalParmString("operation");
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

std::vector<enzo::prm::Template> GopBoolean::parameterList()
{
    using namespace enzo::prm;
    return {
        Template(Type::DROPDOWN, Name("operation", "Operation"), Default("union"))
            .setOptions({
                Name("union", "Union"),
                Name("intersect", "Intersect"),
                Name("subtract", "Subtract"),
            }),
    };
}
