#include "OpDefs/GopCircle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/MeshShapes.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Primitives/Mesh.h"
#include <Eigen/Geometry>
#include <memory>
#include <numbers>
#include <vector>

GopCircle::GopCircle(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopCircle::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    NodePacket packet;
    std::shared_ptr<geo::Mesh> mesh = std::make_shared<geo::Mesh>();

    int numPoints = context.evalParmInt("divisions");
    float uniform_scale = context.evalParmFloat("uniform_scale");

    std::vector<enzo::Vector3> newPointPositions;
    newPointPositions.reserve(numPoints);

    for (int i = 0; i < numPoints; ++i)
    {
        float delta = static_cast<float>(i) / numPoints * std::numbers::pi * 2;
        newPointPositions.push_back({sin(delta) * uniform_scale, 0, cos(delta) * uniform_scale});
    }
    auto newPoints = mesh->addPoints(newPointPositions);
    mesh->addFace(newPoints);

    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

std::vector<enzo::prm::Template> GopCircle::parameterList()
{
    using namespace enzo::prm;
    return {
        enzo::prm::Template(
            enzo::prm::Type::FLOAT,
            enzo::prm::Name("uniform_scale", "Uniform Scale"),
            enzo::prm::Default(1),
            1,
            enzo::prm::Range(0, 10, enzo::prm::RangeFlag::UNLOCKED, enzo::prm::RangeFlag::UNLOCKED)
        ),
        enzo::prm::Template(
            enzo::prm::Type::INT,
            enzo::prm::Name("divisions", "Divisions"),
            enzo::prm::Default(10),
            1,
            enzo::prm::Range(1, 100, enzo::prm::RangeFlag::LOCKED, enzo::prm::RangeFlag::UNLOCKED)
        ),

    };
}
