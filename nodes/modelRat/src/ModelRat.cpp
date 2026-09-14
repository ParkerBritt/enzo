#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/ObjReader.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <filesystem>
#include <fstream>

namespace {

class ModelRat : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void ModelRat::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    const bool isComplex = evalParmString("detail") == "complex";
    const std::filesystem::path modelFile =
        getNodeFolder() / (isComplex ? "ratComplex.obj" : "ratSimple.obj");

    auto mesh = std::make_shared<geo::Mesh>();

    std::ifstream stream(modelFile);
    if (stream.is_open())
        utils::readObjInto(stream, *mesh);
    else
        throwError("Couldn't open " + modelFile.string());

    const Vector3 translate = evalParmVector3("translate");
    const Vector3 rotate = evalParmVector3("rotate");
    const Vector3 scale = evalParmVector3("scale");
    const floatT uniformScale = evalParmFloat("uniform_scale");

    const enzo::Transform transform =
        enzo::Transform::fromComponents(translate, rotate, scale * uniformScale);
    mesh->applyTransform(transform, TransformClass::POINT);

    NodePacket packet;
    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(modelRat, ModelRat)
