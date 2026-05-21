#include "Engine/Utils/MeshUtils.h"

namespace enzo::utils {

bt::Vector3 polygonNormal(std::span<const bt::Vector3> positions,
                          std::span<const bt::intT> polygonPoints)
{
    bt::Vector3 normal(0, 0, 0);
    const size_t count = polygonPoints.size();
    for(size_t cornerIndex=0; cornerIndex<count; ++cornerIndex)
    {
        const bt::Vector3& cur = positions[polygonPoints[cornerIndex]];
        const bt::Vector3& nxt = positions[polygonPoints[(cornerIndex+1)%count]];
        normal.x() += (cur.y() - nxt.y()) * (cur.z() + nxt.z());
        normal.y() += (cur.z() - nxt.z()) * (cur.x() + nxt.x());
        normal.z() += (cur.x() - nxt.x()) * (cur.y() + nxt.y());
    }
    normal.normalize();
    return normal;
}

} // namespace enzo::utils
