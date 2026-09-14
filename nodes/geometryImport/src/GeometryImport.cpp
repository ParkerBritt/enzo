#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/ObjReader.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <fstream>

namespace {

class GeometryImport : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;

  private:
    // Fills the mesh from the file the parameters name, leaving it empty and
    // reporting an error when there is nothing readable to fill it from.
    void readFileInto(enzo::geo::Mesh& mesh);
};

void GeometryImport::readFileInto(enzo::geo::Mesh& mesh)
{
    using namespace enzo;

    String filePath = evalParmString("filePath");
    boost::trim(filePath);

    // An unset path is the state a freshly created node is in, so it stays quiet
    // and hands out empty geometry until the user points it somewhere.
    if (filePath.empty()) return;

    const std::filesystem::path file(filePath);
    if (file.extension() != ".obj")
    {
        throwError("Only obj files can be imported, not " + file.extension().string());
        return;
    }

    std::ifstream stream(file);
    if (!stream.is_open())
    {
        throwError("Couldn't open " + filePath);
        return;
    }

    utils::readObjInto(stream, mesh);

    mesh.applyTransform(Transform().scale(evalParmFloat("size")), TransformClass::POINT);
}

void GeometryImport::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    auto mesh = std::make_shared<geo::Mesh>();
    readFileInto(*mesh);

    NodePacket packet;
    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(geometryImport, GeometryImport)
