#include "Engine/GeometryAlgorithms/ObjReader.h"
#include "Engine/Core/Types.h"
#include "Engine/Primitives/Mesh.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>

namespace enzo::utils {

namespace {

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
bool getPointOffset(const std::string& field, Offset pointCount, Offset& pointOffset)
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

    if (objIndex > 0 && static_cast<Offset>(objIndex) <= pointCount)
    {
        pointOffset = objIndex - 1;
        return true;
    }
    if (objIndex < 0 && static_cast<Offset>(-objIndex) <= pointCount)
    {
        pointOffset = pointCount - static_cast<Offset>(-objIndex);
        return true;
    }
    return false;
}

} // namespace

void readObjInto(std::istream& file, geo::Mesh& mesh)
{
    std::string line;
    while (std::getline(file, line))
    {
        const std::vector<std::string> fields = splitFields(line);
        if (fields.empty()) continue;

        const std::string& record = fields.front();

        if (record == "v" && fields.size() >= 4)
        {
            mesh.addPoint(Vector3(std::stof(fields[1]), std::stof(fields[2]), std::stof(fields[3])));
        }
        else if (record == "f" || record == "l")
        {
            std::vector<Offset> pointOffsets;
            pointOffsets.reserve(fields.size() - 1);

            const Offset pointCount = mesh.getNumPoints();
            for (size_t field = 1; field < fields.size(); ++field)
            {
                Offset pointOffset = 0;
                if (getPointOffset(fields[field], pointCount, pointOffset))
                    pointOffsets.push_back(pointOffset);
            }

            if (pointOffsets.size() >= 2) mesh.addFace(pointOffsets, record == "f");
        }
    }
}

} // namespace enzo::utils
