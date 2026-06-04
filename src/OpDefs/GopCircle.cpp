#include "OpDefs/GopCircle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/MeshShapes.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Primitives/Mesh.h"
#include <Eigen/Geometry>
#include <memory>
#include <numbers>

GopCircle::GopCircle(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopCircle::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    NodePacket packet;
    std::shared_ptr<geo::Mesh> mesh = std::make_shared<geo::Mesh>();
    
    int numPoints = 10;
    for(int i=0; i<numPoints; ++i)
    {
        float delta = static_cast<float>(i)/numPoints*std::numbers::pi*2;
        mesh->addPoint({sin(delta),0,cos(delta)});
    }

    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

std::vector<enzo::prm::Template> GopCircle::parameterList()
{
    using namespace enzo::prm;
    return {

    };
}
