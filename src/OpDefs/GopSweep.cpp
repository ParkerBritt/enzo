#include "OpDefs/GopSweep.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/MeshUtils.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Primitives/Mesh.h"
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Geometry/AngleAxis.h>
#include <numbers>

GopSweep::GopSweep(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopSweep::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if (outputRequested(0))
    {
        NodePacket packet = context.cloneInputPacket(0);

        // Build profile
        std::vector<enzo::Vector3> profilePositions;

        constexpr int circleDivisions = 10;
        profilePositions.reserve(10);

        for (int i = 0; i < circleDivisions; ++i)
        {
            float delta = static_cast<float>(i) / (circleDivisions - 1) * 2 * std::numbers::pi;
            float x = sin(delta);
            float z = cos(delta);
            profilePositions.push_back({x, 0, z});
        }

        const int profileNumPoints = profilePositions.size();

        // TODO: too many indentation levels, should do something about that
        for (auto prim : packet.getPrimitives())
        {
            std::shared_ptr<enzo::geo::Mesh> mesh =
                std::dynamic_pointer_cast<enzo::geo::Mesh>(prim);
            if (!mesh) continue;

            std::vector<enzo::Vector3> pointPositions;
            pointPositions.reserve(circleDivisions * prim->getNumPoints());
            enzo::utils::FaceTangents tangents(*mesh);

            // Add profile points to each point on source curve
            for (enzo::Offset faceOffset : mesh->getFaces())
            {
                const int numCurvePoints = mesh->getFacePointCount(faceOffset);
                const auto faceTangents = tangents(faceOffset);

                for (auto point : mesh->getFacePoints(faceOffset))
                {
                    const float curveU = relPointNum / (numCurvePoints - 1);
                    const enzo::Vector3 tangent = faceTangents[point];

                    // Build transform
                    enzo::Transform transform;
                    transform.scale(curveU); // TODO:  control with ramp parameter
                    transform.lookAt(tangent, normal);
                    transform.translate(mesh->getPointPos(point));

                    // Add profile points
                    for (auto profilePos : profilePositions)
                    {
                        pointPositions.push_back(profilePos * transform);
                    }
                }
            }
            std::vector<enzo::Offset> newPoints = mesh->addPoints(pointPositions);

            // Delete original faces
            mesh->deleteAllFaces();

            // Connect faces
            std::vector<enzo::Offset> faceOffsets;
            std::vector<enzo::Offset> faceVertexCounts;

            for (size_t i = 0; i < newPoints.size() - profileNumPoints - 1; ++i)
            {
                const enzo::Offset point1 = newPoints[i];
                const enzo::Offset point2 = newPoints[i + 1];
                const enzo::Offset point3 = newPoints[i + 1 + profileNumPoints];
                const enzo::Offset point4 = newPoints[i + profileNumPoints];

                faceOffsets.push_back(point1);
                faceOffsets.push_back(point2);
                faceOffsets.push_back(point3);
                faceOffsets.push_back(point4);

                faceVertexCounts.push_back(4);
            }

            // faceOffsets.reserve();
            // faceVertexCounts.reserve();

            mesh->addFaces(faceOffsets, faceVertexCounts);
        }
        setOutputPacket(0, packet);
    }
}

std::vector<enzo::prm::Template> GopSweep::parameterList() { return {}; }
