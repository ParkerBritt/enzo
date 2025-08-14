#include "OpDefs/GopHouse.h"
#include "Engine/Operator/AttributeHandle.h"
#include <tbb/parallel_for.h>

GOP_house::GOP_house(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{

}

void GOP_house::cookOp(enzo::op::Context context)
{
    using namespace enzo;
    // std::cout << "COOKING\n";

    if(outputRequested(0))
    {
        // copy input geometry
        geo::Geometry geo;

        // ----
        // create geometry start
        // ----
        auto PAttr = geo.getAttribByName(ga::AttrOwner::POINT, "P");
        ga::AttributeHandleVector3 PAttrHandle(PAttr);
        int startPt = PAttrHandle.getSize();
        std::vector<bt::Vector3> pts = {
            {-1,-1,-1},
            {1,-1,-1},
            {1,-1,1},
            {-1,-1,1},
            {-1,1,-1},
            {1,1,-1},
            {1,1,1},
            {-1,1,1},
            {0,2,-1},
            {0,2,1}
        };
        std::vector<std::vector<int>> faces = {
            {7,9,6,2,3},
            {4,8,5,1,0},
            {4,7,3,0},
            {5,6,2,1},
            {0,1,2,3},
            {9,7,4},
            {8,9,4},
            {9,6,5},
            {8,5,9}
        };

        for (auto& p : pts) PAttrHandle.addValue(p);

        auto pointAttr = geo.getAttribByName(ga::AttrOwner::VERTEX, "point");
        ga::AttributeHandleInt pointAttrHandle(pointAttr);
        for (auto& f : faces) for (int i : f) pointAttrHandle.addValue(startPt + i);

        auto vertexCountAttr = geo.getAttribByName(ga::AttrOwner::PRIMITIVE, "vertexCount");
        ga::AttributeHandleInt vertexCountHandle(vertexCountAttr);
        for (auto& f : faces) vertexCountHandle.addValue(f.size());

        // --------

        for(int i=0; i<PAttrHandle.getAllValues().size(); ++i)
        {
            enzo::bt::Vector3 vector = PAttrHandle.getValue(i);
            vector*=context.evalFloatParm("size");
            PAttrHandle.setValue(i, vector);
        }
        // ----

        // set output geometry
        setOutputGeometry(0, geo);
    }

}

enzo::prm::Template GOP_house::parameterList[] =
{
    enzo::prm::Template(enzo::prm::Type::FLOAT, enzo::prm::Name("size", "Size"), 1),
    enzo::prm::Terminator
};
