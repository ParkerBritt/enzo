#include "OpDefs/GopTestCube.h"
#include "Engine/Operator/AttributeHandle.h"
#include <oneapi/tbb/parallel_for.h>

GopTestGeoCube::GopTestGeoCube(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopTestGeoCube::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if (outputRequested(0))
    {
        geo::Geometry geo = context.cloneInputGeo(0);

        auto PAttr = geo.getAttribByName(ga::AttrOwner::POINT, "P");
        ga::AttributeHandleVector3 PAttrHandle(PAttr);
        const int startPt = PAttrHandle.getSize();

        const std::vector<bt::Vector3> pts = {
            {-1, -1,  1},
            { 1, -1,  1},
            { 1,  1,  1},
            {-1,  1,  1},
            {-1, -1, -1},
            { 1, -1, -1},
            { 1,  1, -1},
            {-1,  1, -1} 
        };

        for (const auto &p : pts) PAttrHandle.addValue(p);

        const std::vector<std::vector<int>> faces = {
            {0, 1, 2, 3},
            {7, 6, 5, 4},
            {0, 3, 7, 4},
            {5, 6, 2, 1},
            {3, 2, 6, 7},
            {4, 5, 1, 0} 
        };

        auto pointAttr = geo.getAttribByName(ga::AttrOwner::VERTEX, "point");
        ga::AttributeHandleInt pointAttrHandle(pointAttr);
        for (const auto &f : faces)
            for (int i : f)
                pointAttrHandle.addValue(startPt + i);

        auto vertexCountAttr = geo.getAttribByName(ga::AttrOwner::PRIMITIVE,
                                                  "vertexCount");
        ga::AttributeHandleInt vertexCountHandle(vertexCountAttr);
        for (const auto &f : faces)
            vertexCountHandle.addValue(static_cast<int>(f.size()));

        const float s = context.evalFloatParm("size");
        for (int i = 0; i < PAttrHandle.getAllValues().size(); ++i)
        {
            bt::Vector3 v = PAttrHandle.getValue(i);
            v *= s;
            PAttrHandle.setValue(i, v);
        }

        setOutputGeometry(0, geo);
    }
}

enzo::prm::Template GopTestGeoCube::parameterList[] =
{
    enzo::prm::Template(enzo::prm::Type::FLOAT, enzo::prm::Name("size", "Size"), 1),
    enzo::prm::Terminator
};
