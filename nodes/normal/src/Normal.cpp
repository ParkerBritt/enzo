#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/Normals.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <memory>
#include <vector>

namespace {

/// @brief Returns the named vector attribute, creating it when the mesh has none.
enzo::attr::AttributeHandle<enzo::Vector3> getOrAddVector3Attribute(
    enzo::geo::Mesh& mesh,
    enzo::attr::AttributeOwner owner,
    const std::string& name
)
{
    std::shared_ptr<enzo::attr::Attribute> existing = mesh.getAttribByName(owner, name);
    if (existing) return enzo::attr::AttributeHandle<enzo::Vector3>(existing);
    return mesh.addVector3Attribute(owner, name);
}

class Normal : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Normal::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = cloneInputPacket(0);

    const bool perVertex = evalParmString("attachTo") == "vertex";
    const double cuspAngle = evalParmFloat("cuspAngle");
    const String attributeName = evalParmString("name");

    for (geo::PrimPtr prim : packet.getPrimitives())
    {
        auto mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
        if (!mesh) continue;

        const std::vector<Vector3> normals = perVertex
                                                 ? utils::computeVertexNormals(*mesh, cuspAngle)
                                                 : utils::computePointNormals(*mesh);

        const attr::AttrOwner owner = perVertex ? attr::AttrOwner::VERTEX : attr::AttrOwner::POINT;
        attr::AttributeHandle<Vector3> normalAttribute =
            getOrAddVector3Attribute(*mesh, owner, attributeName);

        for (Offset offset = 0; offset < normals.size(); ++offset)
        {
            normalAttribute.setValue(offset, normals[offset]);
        }
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(normal, Normal)
