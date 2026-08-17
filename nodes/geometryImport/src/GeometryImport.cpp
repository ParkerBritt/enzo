#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <fstream>
#include <istream>
#include <string>
#include <vector>

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

// Splits an obj line on runs of whitespace, dropping the empty fields that
// leading or doubled spaces would otherwise produce.
std::vector<std::string> splitFields(const std::string& line)
{
    std::vector<std::string> fields;
    boost::split(fields, line, boost::is_any_of(" \t\r"), boost::token_compress_on);
    fields.erase(
        std::remove_if(
            fields.begin(),
            fields.end(),
            [](const std::string& field) { return field.empty(); }
        ),
        fields.end()
    );
    return fields;
}

// Resolves one vertex reference of a face to a point offset.
//
//     "12"      -> 11
//     "12/4/7"  -> 11
//     "-1"      -> the point added most recently
//
// Obj counts vertices from one and lets a negative index count back from the
// end, so both forms land on the same zero based offset the mesh uses.
// Returns false when the field is not a number at all.
bool getPointOffset(const std::string& field, enzo::Offset pointCount, enzo::Offset& pointOffset)
{
    const std::string indexText = field.substr(0, field.find('/'));

    int objIndex = 0;
    try
    {
        objIndex = std::stoi(indexText);
    }
    catch (const std::exception&)
    {
        return false;
    }

    if (objIndex > 0 && static_cast<enzo::Offset>(objIndex) <= pointCount)
    {
        pointOffset = objIndex - 1;
        return true;
    }
    if (objIndex < 0 && static_cast<enzo::Offset>(-objIndex) <= pointCount)
    {
        pointOffset = pointCount - static_cast<enzo::Offset>(-objIndex);
        return true;
    }
    return false;
}

// Reads the vertex and face records of an obj file into a mesh, ignoring the
// records enzo has nowhere to put such as materials, normals, and texture
// coordinates. A "f" record closes its face and an "l" record leaves it open.
void readObjInto(std::istream& file, enzo::geo::Mesh& mesh)
{
    std::string line;
    while (std::getline(file, line))
    {
        const std::vector<std::string> fields = splitFields(line);
        if (fields.empty()) continue;

        const std::string& record = fields.front();

        if (record == "v" && fields.size() >= 4)
        {
            mesh.addPoint(enzo::Vector3(
                std::stof(fields[1]),
                std::stof(fields[2]),
                std::stof(fields[3])
            ));
        }
        else if (record == "f" || record == "l")
        {
            std::vector<enzo::Offset> pointOffsets;
            pointOffsets.reserve(fields.size() - 1);

            const enzo::Offset pointCount = mesh.getNumPoints();
            for (size_t field = 1; field < fields.size(); ++field)
            {
                enzo::Offset pointOffset = 0;
                if (getPointOffset(fields[field], pointCount, pointOffset))
                    pointOffsets.push_back(pointOffset);
            }

            if (pointOffsets.size() >= 2) mesh.addFace(pointOffsets, record == "f");
        }
    }
}

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

    readObjInto(stream, mesh);

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
