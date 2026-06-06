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
    std::string arc_type = context.evalParmString("arc");
    float arcBegin = context.evalParmFloat("arc_angles", 0) / 360;
    float arcEnd = context.evalParmFloat("arc_angles", 1) / 360;
    enzo::Vector3 center = context.evalParmVector3("center");
    enzo::Vector3 rotate = context.evalParmVector3("rotate");
    enzo::Vector2 radius = context.evalParmVector2("radius");

    float arcDelta = arcEnd - arcBegin;
    if (arc_type == "closed") arcDelta = 1;

    const bool isArc = (arc_type != "closed");

    numPoints += isArc;

    std::vector<enzo::Vector3> newPointPositions;
    newPointPositions.reserve(numPoints);

    // Sliced arc has one extra point in the middle for the triangles to anchor to
    if (arc_type == "sliced_arc") newPointPositions.push_back({0, 0, 0});

    for (int divisionIndex = 0; divisionIndex < numPoints; ++divisionIndex)
    {
        float pointU = static_cast<float>(divisionIndex) / (numPoints - 1 * isArc);
        float angle = (arcBegin + pointU * arcDelta) * std::numbers::pi * 2;
        float u = sin(angle) * uniform_scale * radius[0];
        float v = cos(angle) * uniform_scale * radius[1];

        enzo::Vector3 position;
        if (orientation == "xy")
            position = {u, v, 0};
        else if (orientation == "yz")
            position = {0, u, v};
        else
            position = {u, 0, v};

        newPointPositions.push_back(position);
    }

    // Create points of circle
    auto newPoints = mesh->addPoints(newPointPositions);

    // Sliced arc gets one face for every two points, creating triangles
    if (arc_type == "sliced_arc")
    {
        for (size_t pointOffset = 1; pointOffset < newPoints.size() - 1; ++pointOffset)
        {
            mesh->addFace({newPoints[pointOffset], newPoints[pointOffset + 1], newPoints[0]});
        }
    }
    else
    {
        bool closed = arc_type != "open_arc";
        mesh->addFace(newPoints, closed);
    }

    mesh->applyTransform(enzo::Transform().translate(center).rotateEuler(rotate));

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
            enzo::prm::Type::XYZ,
            enzo::prm::Name("center", "Center"),
            enzo::prm::Default(0),
            3,
            enzo::prm::Range(
                -10,
                10,
                enzo::prm::RangeFlag::UNLOCKED,
                enzo::prm::RangeFlag::UNLOCKED
            )
        ),
        enzo::prm::Template(
            enzo::prm::Type::XYZ,
            enzo::prm::Name("rotate", "Rotate"),
            enzo::prm::Default(0),
            3,
            enzo::prm::Range(
                -360,
                360,
                enzo::prm::RangeFlag::UNLOCKED,
                enzo::prm::RangeFlag::UNLOCKED
            )
        ),
        enzo::prm::Template(
            enzo::prm::Type::FLOAT,
            enzo::prm::Name("uniform_scale", "Uniform Scale"),
            enzo::prm::Default(1),
            1,
            enzo::prm::Range(0, 10, enzo::prm::RangeFlag::UNLOCKED, enzo::prm::RangeFlag::UNLOCKED)
        ),
        enzo::prm::Template(
            enzo::prm::Type::XYZ,
            enzo::prm::Name("radius", "Radius"),
            enzo::prm::Default(1),
            2,
            enzo::prm::Range(0, 10, enzo::prm::RangeFlag::UNLOCKED, enzo::prm::RangeFlag::UNLOCKED)
        ),
        enzo::prm::Template(
            enzo::prm::Type::INT,
            enzo::prm::Name("divisions", "Divisions"),
            enzo::prm::Default(10),
            1,
            enzo::prm::Range(1, 100, enzo::prm::RangeFlag::LOCKED, enzo::prm::RangeFlag::UNLOCKED)
        ),
        enzo::prm::Template(enzo::prm::Type::GROUP, enzo::prm::Name("arc_group", "Arc"))
            .setBackgroundEnabled(false)
            .addParm(
                enzo::prm::Template(enzo::prm::Type::DROPDOWN, enzo::prm::Name("arc", "Arc Type"))
                    .setLabelHidden(true)
                    .setOptions({
                        Name("closed", "Closed"),
                        Name("open_arc", "Open Arc"),
                        Name("closed_arc", "Closed Arc"),
                        Name("sliced_arc", "Sliced Arc"),
                    })
            )
            .addParm(
                enzo::prm::Template(
                    enzo::prm::Type::XYZ,
                    enzo::prm::Name("arc_angles", "Arc Angles"),
                    {enzo::prm::Default(0), enzo::prm::Default(360)},
                    2,
                    {enzo::prm::Range(
                        0,
                        360,
                        enzo::prm::RangeFlag::LOCKED,
                        enzo::prm::RangeFlag::UNLOCKED
                    )}
                )
                    .setLabelHidden(true)
            )
    };
}
