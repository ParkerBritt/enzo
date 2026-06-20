#include "OpDefs/GopHouse.h"
#include "Engine/Attribute/AttributeHandle.h"
#include <tbb/parallel_for.h>

GOP_house::GOP_house(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GOP_house::cookOp(enzo::op::Context context)
{
    using namespace enzo;
    // std::cout << "COOKING\n";

    if (outputRequested(0))
    {
        // TODO: convert to NodePacket
        NodePacket packet;
        // geo::Primitive geo;
        //
        // auto PAttr = geo.getAttribByName(attr::AttrOwner::POINT, "P");
        // attr::AttributeHandleVector3 PAttrHandle(PAttr);
        // int startPt = PAttrHandle.getSize();
        // std::vector<Vector3> pts = {
        //     {-1,-1,-1},
        //     {1,-1,-1},
        //     {1,-1,1},
        //     {-1,-1,1},
        //     {-1,1,-1},
        //     {1,1,-1},
        //     {1,1,1},
        //     {-1,1,1},
        //     {0,2,-1},
        //     {0,2,1}
        // };
        // std::vector<std::vector<int>> faces = {
        //     {7,9,6,2,3},
        //     {4,8,5,1,0},
        //     {4,7,3,0},
        //     {5,6,2,1},
        //     {0,1,2,3},
        //     {9,7,4},
        //     {8,9,4},
        //     {9,6,5},
        //     {8,5,9}
        // };
        //
        // for (auto& p : pts) PAttrHandle.addValue(p);
        //
        // auto pointAttr = geo.getAttribByName(attr::AttrOwner::VERTEX, "point");
        // attr::AttributeHandleInt pointAttrHandle(pointAttr);
        // for (auto& f : faces) for (int i : f) pointAttrHandle.addValue(startPt + i);
        //
        // auto vertexCountAttr = geo.getAttribByName(attr::AttrOwner::PRIMITIVE, "vertexCount");
        // attr::AttributeHandleInt vertexCountHandle(vertexCountAttr);
        // for (auto& f : faces) vertexCountHandle.addValue(f.size());
        //
        // for(int i=0; i<PAttrHandle.getAllValues().size(); ++i)
        // {
        //     enzo::Vector3 vector = PAttrHandle.getValue(i);
        //     vector*=context.evalParmFloat("size");
        //     PAttrHandle.setValue(i, vector);
        // }

        setOutputPacket(0, packet);
    }
}

std::vector<enzo::prm::Template> GOP_house::parameterList()
{
    return {enzo::prm::Template(enzo::prm::Type::FLOAT, enzo::prm::Name("size", "Size"), 1)};
}
