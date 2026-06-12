#include "OpDefs/GopSweep.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
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

        for (auto prim : packet.getPrimitives())
        {
            std::shared_ptr<enzo::geo::Mesh> mesh =
                std::dynamic_pointer_cast<enzo::geo::Mesh>(prim);
            if (!mesh) continue;

            // Place points
            std::vector<enzo::Vector3> pointPositions;
            pointPositions.reserve(circleDivisions * prim->getNumPoints());

            for (auto point : prim->getPoints())
            {
                for (auto profilePos : profilePositions)
                {
                    pointPositions.push_back(profilePos + mesh->getPointPos(point));
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
