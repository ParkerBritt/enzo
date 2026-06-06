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
    std::string orientation = context.evalParmString("orientation");

    std::vector<enzo::Vector3> newPointPositions;
    newPointPositions.reserve(numPoints);

    for (int divisionIndex = 0; divisionIndex < numPoints; ++divisionIndex)
    {
        float delta = static_cast<float>(divisionIndex) / numPoints * std::numbers::pi * 2;
        float u = sin(delta) * uniform_scale;
        float v = cos(delta) * uniform_scale;

        enzo::Vector3 position;
        if (orientation == "xy")
            position = {u, v, 0};
        else if (orientation == "yz")
            position = {0, u, v};
        else
            position = {u, 0, v};

        newPointPositions.push_back(position);
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
            enzo::prm::Type::DROPDOWN,
            enzo::prm::Name("orientation", "Orientation"),
            enzo::prm::Default("zx")
        )
            .setOptions({
                Name("xy", "XY Plane"),
                Name("yz", "YZ Plane"),
                Name("zx", "ZX Plane"),
            }),
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
