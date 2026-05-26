#include "OpDefs/GopCube.h"
#include "Engine/Operator/Mesh.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <Eigen/Geometry>
#include <tuple>
#include <unordered_map>

namespace
{
using GridIndex = std::tuple<int, int, int>;
struct GridIndexHash
{
    size_t operator()(const GridIndex& key) const noexcept
    {
        const size_t hashI = std::hash<int>{}(std::get<0>(key));
        const size_t hashJ = std::hash<int>{}(std::get<1>(key));
        const size_t hashK = std::hash<int>{}(std::get<2>(key));
        size_t combined = hashI;
        combined ^= hashJ + 0x9e3779b97f4a7c15ULL + (combined << 6) + (combined >> 2);
        combined ^= hashK + 0x9e3779b97f4a7c15ULL + (combined << 6) + (combined >> 2);
        return combined;
    }
};
}

GopCube::GopCube(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{
}

void GopCube::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet;
    auto mesh = std::make_shared<geo::Mesh>();

    const bt::floatT sizeX = context.evalFloatParm("size", 0);
    const bt::floatT sizeY = context.evalFloatParm("size", 1);
    const bt::floatT sizeZ = context.evalFloatParm("size", 2);

    const bt::floatT centerX = context.evalFloatParm("center", 0);
    const bt::floatT centerY = context.evalFloatParm("center", 1);
    const bt::floatT centerZ = context.evalFloatParm("center", 2);

    const bt::floatT uniformScale = context.evalFloatParm("uniformScale");

    const bt::floatT rotateX = context.evalFloatParm("rotate", 0) * M_PI / 180.0;
    const bt::floatT rotateY = context.evalFloatParm("rotate", 1) * M_PI / 180.0;
    const bt::floatT rotateZ = context.evalFloatParm("rotate", 2) * M_PI / 180.0;

    Eigen::Affine3d rotation = Eigen::Affine3d::Identity();
    rotation.rotate(Eigen::AngleAxisd(rotateX, Eigen::Vector3d::UnitX()));
    rotation.rotate(Eigen::AngleAxisd(rotateY, Eigen::Vector3d::UnitY()));
    rotation.rotate(Eigen::AngleAxisd(rotateZ, Eigen::Vector3d::UnitZ()));
    const Eigen::Matrix3d rotationMatrix = rotation.linear();

    const bt::intT divisionsX = 1;
    const bt::intT divisionsY = 1;
    const bt::intT divisionsZ = 1;

    // Allocate only the points that lie on the cube surface, keyed by integer grid coordinate.
    std::unordered_map<GridIndex, ga::Offset, GridIndexHash> gridToOffset;

    auto pointAt = [&](int i, int j, int k) -> ga::Offset
    {
        const GridIndex key{i, j, k};
        auto it = gridToOffset.find(key);
        if (it != gridToOffset.end()) return it->second;

        const bt::floatT fx = static_cast<bt::floatT>(i) / static_cast<bt::floatT>(divisionsX);
        const bt::floatT fy = static_cast<bt::floatT>(j) / static_cast<bt::floatT>(divisionsY);
        const bt::floatT fz = static_cast<bt::floatT>(k) / static_cast<bt::floatT>(divisionsZ);

        const Eigen::Vector3d localPos(
            (fx - 0.5) * sizeX * uniformScale,
            (fy - 0.5) * sizeY * uniformScale,
            (fz - 0.5) * sizeZ * uniformScale);

        const Eigen::Vector3d rotated = rotationMatrix * localPos;
        const bt::Vector3 pos(centerX + rotated.x(), centerY + rotated.y(), centerZ + rotated.z());

        const ga::Offset offset = mesh->addPoint(pos);
        gridToOffset.emplace(key, offset);
        return offset;
    };

    // Bottom and top faces span the X and Z grid.
    for (int i = 0; i < divisionsX; ++i)
    {
        for (int k = 0; k < divisionsZ; ++k)
        {
            const ga::Offset bottomA = pointAt(i,     0, k);
            const ga::Offset bottomB = pointAt(i + 1, 0, k);
            const ga::Offset bottomC = pointAt(i + 1, 0, k + 1);
            const ga::Offset bottomD = pointAt(i,     0, k + 1);
            mesh->addFace({bottomA, bottomB, bottomC, bottomD});

            const ga::Offset topA = pointAt(i,     divisionsY, k);
            const ga::Offset topB = pointAt(i + 1, divisionsY, k);
            const ga::Offset topC = pointAt(i + 1, divisionsY, k + 1);
            const ga::Offset topD = pointAt(i,     divisionsY, k + 1);
            mesh->addFace({topA, topD, topC, topB});
        }
    }

    // Back and front faces span the X and Y grid.
    for (int i = 0; i < divisionsX; ++i)
    {
        for (int j = 0; j < divisionsY; ++j)
        {
            const ga::Offset backA = pointAt(i,     j,     0);
            const ga::Offset backB = pointAt(i + 1, j,     0);
            const ga::Offset backC = pointAt(i + 1, j + 1, 0);
            const ga::Offset backD = pointAt(i,     j + 1, 0);
            mesh->addFace({backA, backD, backC, backB});

            const ga::Offset frontA = pointAt(i,     j,     divisionsZ);
            const ga::Offset frontB = pointAt(i + 1, j,     divisionsZ);
            const ga::Offset frontC = pointAt(i + 1, j + 1, divisionsZ);
            const ga::Offset frontD = pointAt(i,     j + 1, divisionsZ);
            mesh->addFace({frontA, frontB, frontC, frontD});
        }
    }

    // Left and right faces span the Y and Z grid.
    for (int j = 0; j < divisionsY; ++j)
    {
        for (int k = 0; k < divisionsZ; ++k)
        {
            const ga::Offset leftA = pointAt(0, j,     k);
            const ga::Offset leftB = pointAt(0, j + 1, k);
            const ga::Offset leftC = pointAt(0, j + 1, k + 1);
            const ga::Offset leftD = pointAt(0, j,     k + 1);
            mesh->addFace({leftA, leftD, leftC, leftB});

            const ga::Offset rightA = pointAt(divisionsX, j,     k);
            const ga::Offset rightB = pointAt(divisionsX, j + 1, k);
            const ga::Offset rightC = pointAt(divisionsX, j + 1, k + 1);
            const ga::Offset rightD = pointAt(divisionsX, j,     k + 1);
            mesh->addFace({rightA, rightB, rightC, rightD});
        }
    }

    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

std::vector<enzo::prm::Template> GopCube::parameterList()
{
    using namespace enzo::prm;
    return {
        Template(Type::XYZ, Name("size", "Size"), Default(1), 3, Range(0, 100)),
        Template(Type::XYZ, Name("center", "Center"), Default(0), 3),
        Template(Type::XYZ, Name("rotate", "Rotation"), Default(0), 3),
        Template(Type::FLOAT, Name("uniformScale", "Uniform Scale"), Default(1), 1, Range(0, 10)),
    };
}
