#include "OpDefs/GopExtrude.h"
#include "Engine/Operator/Mesh.h"
#include "Engine/Operator/Selection.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <vector>
#include <icecream.hpp>

void extrude(enzo::geo::PrimPtr prim, std::vector<enzo::ga::Offset> faces, float distance=1)
{
    std::shared_ptr<enzo::geo::Mesh> mesh = std::dynamic_pointer_cast<enzo::geo::Mesh>(prim);
    if(!mesh)
    {
        return;
    }

    const std::string sideGroupName = "extrudeSide";
    mesh->createFaceGroup(sideGroupName);

    std::vector<enzo::ga::Offset> sidePointOffsetsFlat;
    std::vector<enzo::ga::Offset> sideVertexCounts;
    std::vector<enzo::ga::Offset> topPointOffsetsFlat;
    std::vector<enzo::ga::Offset> topVertexCounts;

    auto faceNormals = mesh->getFaceNormal();

    for(auto face : faces)
    {
        auto originFacePoints = mesh->getFacePoints(face);
        const enzo::bt::Vector3 displacement = faceNormals[face] * distance;

        std::vector<enzo::ga::Offset> oldPoints;
        std::vector<enzo::ga::Offset> newPoints;

        // Duplicate each origin point along the face normal
        for(auto pointOffset : originFacePoints)
        {
            auto pointPos = mesh->getPointPos(pointOffset);
            pointPos += displacement;
            enzo::ga::Offset newPoint = mesh->addPoint(pointPos);
            oldPoints.push_back(pointOffset);
            newPoints.push_back(newPoint);
        }

        // Build the side quads connecting the old ring to the new ring
        const size_t numFacePoints = oldPoints.size();
        for(size_t cornerIndex=0; cornerIndex<numFacePoints; ++cornerIndex)
        {
            const size_t nextIndex = (cornerIndex+1) % numFacePoints;
            const enzo::ga::Offset bottomStart = oldPoints[cornerIndex];
            const enzo::ga::Offset bottomEnd = oldPoints[nextIndex];
            const enzo::ga::Offset topEnd = newPoints[nextIndex];
            const enzo::ga::Offset topStart = newPoints[cornerIndex];

            sidePointOffsetsFlat.push_back(bottomStart);
            sidePointOffsetsFlat.push_back(bottomEnd);
            sidePointOffsetsFlat.push_back(topEnd);
            sidePointOffsetsFlat.push_back(topStart);
            sideVertexCounts.push_back(4);
        }

        // Top face caps the extrusion
        for(auto newPoint : newPoints) topPointOffsetsFlat.push_back(newPoint);
        topVertexCounts.push_back(numFacePoints);
    }

    std::vector<enzo::ga::Offset> sideOffsets = mesh->addFaces(sidePointOffsetsFlat, sideVertexCounts);
    mesh->addToFaceGroup(sideGroupName, sideOffsets);

    mesh->addFaces(topPointOffsetsFlat, topVertexCounts);
}

GopExtrude::GopExtrude(enzo::nt::NetworkManager *network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo) {}

void GopExtrude::cookOp(enzo::op::Context context) {
    using namespace enzo;

    if (outputRequested(0)) {
        NodePacket packet = context.cloneInputPacket(0);

        const bt::String selectionStr = context.evalStringParm("selection");
        const float distance = context.evalFloatParm("distance");
        enzo::Selection selection(selectionStr);

        for (geo::PrimPtr prim : selection.getPrims(packet)) {
            extrude(prim, selection.getFaces(prim), distance);
        }

        setOutputPacket(0, packet);
    }
}

enzo::prm::Template GopExtrude::parameterList[] = {
    enzo::prm::Template(enzo::prm::Type::STRING, enzo::prm::Name("selection", "Selection")),
    enzo::prm::Template(enzo::prm::Type::FLOAT, enzo::prm::Name("distance", "Distance"), enzo::prm::Default(1), 1, enzo::prm::Range(-10, 10)),
    enzo::prm::Terminator};
