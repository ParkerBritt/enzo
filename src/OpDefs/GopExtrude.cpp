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
#include <unordered_set>
#include <vector>
#include <icecream.hpp>

void extrude(enzo::geo::PrimPtr prim, std::vector<enzo::ga::Offset> faces, float distance=1)
{
    std::shared_ptr<enzo::geo::Mesh> mesh = std::dynamic_pointer_cast<enzo::geo::Mesh>(prim);
    if(!mesh)
    {
        return;
    }

    std::unordered_set<enzo::ga::Offset> duplicatePoints;

    std::string sideGroupName = "extrudeSide";
    mesh->createGroup(enzo::ga::AttributeOwner::FACE, sideGroupName);

    std::vector<std::vector<enzo::ga::Offset>> newFacePoints;

    for(auto face: faces)
    {
        auto originFacePoints = mesh->getFacePoints(face);

        std::vector<enzo::ga::Offset> oldPoints;
        std::vector<enzo::ga::Offset> newPoints;
        
        // Create points
        for(auto pointOffset : originFacePoints)
        {
            auto pointPos = mesh->getPointPos(pointOffset);
            pointPos.y() += distance;
            enzo::ga::Offset newPoint = mesh->addPoint(pointPos);
            oldPoints.push_back(pointOffset);
            newPoints.push_back(newPoint);
        }

        size_t numFacePoints = oldPoints.size();
        // Connect faces
        for(size_t i=0; i<numFacePoints; ++i)
        {
            size_t nextIndex = (i+1)%numFacePoints;
            enzo::ga::Offset p1 = oldPoints[i];
            enzo::ga::Offset p2 = oldPoints[nextIndex];
            enzo::ga::Offset p3 = newPoints[nextIndex];
            enzo::ga::Offset p4 = newPoints[i];
            // Side face
            newFacePoints.push_back(std::vector<enzo::ga::Offset>{p1, p2, p3, p4});
            // enzo::ga::Offset faceOff = mesh->addFace({p1, p2, p3, p4});
        }

        // Top face
        newFacePoints.push_back(newPoints);
    }

    std::vector<enzo::ga::Offset> sideOffsets = mesh->addFaces(newFacePoints);
    mesh->addToGroup(enzo::ga::AttributeOwner::FACE, sideGroupName, sideOffsets);

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
