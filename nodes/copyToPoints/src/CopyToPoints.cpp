#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include "Engine/Primitives/Primitive.h"
#include <memory>
#include <span>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <type_traits>
#include <vector>

namespace {

using namespace enzo;

class CopyToPoints : public nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

// Runs the body once for every copy index, spread across threads.
template <typename Body> void forEachCopy(Offset copyCount, const Body& body)
{
    tbb::parallel_for(
        tbb::blocked_range<Offset>(0, copyCount),
        [&](const tbb::blocked_range<Offset>& copies) {
            for (Offset copyIndex = copies.begin(); copyIndex < copies.end(); ++copyIndex)
                body(copyIndex);
        }
    );
}

// Writes the source attribute's values once per copy, one block after another.
template <typename T>
void repeatAttributeValues(
    const std::shared_ptr<const attr::Attribute>& source,
    const std::shared_ptr<attr::Attribute>& destination,
    Offset copyCount
)
{
    const std::vector<T> sourceValues = attr::AttributeHandleRO<T>(source).getAllValues();
    attr::AttributeHandle<T> destinationValues(destination);
    const Offset valueCount = sourceValues.size();

    const auto writeCopy = [&](Offset copyIndex) {
        const Offset copyStart = copyIndex * valueCount;
        for (Offset valueIndex = 0; valueIndex < valueCount; ++valueIndex)
            destinationValues.setValue(copyStart + valueIndex, sourceValues[valueIndex]);
    };

    // Writes bools on one thread, since their storage packs neighbouring values into shared bytes.
    if (std::is_same_v<T, boolT>)
    {
        for (Offset copyIndex = 0; copyIndex < copyCount; ++copyIndex)
            writeCopy(copyIndex);
        return;
    }
    forEachCopy(copyCount, writeCopy);
}

void repeatAttribute(
    const std::shared_ptr<const attr::Attribute>& source,
    const std::shared_ptr<attr::Attribute>& destination,
    Offset copyCount
)
{
    switch (source->getType())
    {
    case attr::AttrType::intT:
        repeatAttributeValues<intT>(source, destination, copyCount);
        break;
    case attr::AttrType::floatT:
        repeatAttributeValues<floatT>(source, destination, copyCount);
        break;
    case attr::AttrType::vectorT:
        repeatAttributeValues<Vector3>(source, destination, copyCount);
        break;
    case attr::AttrType::boolT:
        repeatAttributeValues<boolT>(source, destination, copyCount);
        break;
    case attr::AttrType::matrixT:
        repeatAttributeValues<Matrix4>(source, destination, copyCount);
        break;
    default:
        break;
    }
}

// Adds every attribute and group the prototype holds on this owner to the copies and repeats their
// values.
void repeatOwnerAttributes(
    const geo::Mesh& prototype,
    geo::Mesh& copies,
    attr::AttributeOwner owner,
    Offset copyCount
)
{
    copies.addAttributesFrom(prototype, owner);
    for (const auto& sourceAttribute : prototype.getAttributes(owner))
    {
        repeatAttribute(
            sourceAttribute,
            copies.getAttribByName(owner, sourceAttribute->getName()),
            copyCount
        );
    }
    for (const auto& sourceGroup : prototype.getGroups(owner))
    {
        repeatAttribute(sourceGroup, copies.getGroupByName(owner, sourceGroup->getName()), copyCount);
    }
}

// Returns one mesh holding a copy of the prototype placed by each transform.
std::shared_ptr<geo::Mesh>
copyMeshToTransforms(const geo::Mesh& prototype, const std::vector<Transform>& transforms)
{
    const Offset copyCount = transforms.size();
    const Offset prototypePointCount = prototype.getNumPoints();
    auto copies = std::make_shared<geo::Mesh>(prototype.getPath());

    // Points
    const std::span<const Vector3> prototypePositions = prototype.pointPosSpan();
    std::vector<Vector3> positions(copyCount * prototypePointCount);

    // Duplicate the prototype's points for each transform and add them to the mesh.
    forEachCopy(copyCount, [&](Offset copyIndex) {
        const Transform& transform = transforms[copyIndex];
        const Offset copyStart = copyIndex * prototypePointCount;
        for (Offset pointOffset = 0; pointOffset < prototypePointCount; ++pointOffset)
            positions[copyStart + pointOffset] = transform * prototypePositions[pointOffset];
    });
    copies->addPoints(positions);

    // Faces
    std::vector<Offset> prototypeFacePoints;
    std::vector<Offset> prototypeVertexCounts;
    for (const Offset faceOffset : prototype.getFaces())
    {
        const std::span<const intT> facePoints = prototype.getFacePoints(faceOffset);
        prototypeFacePoints.insert(prototypeFacePoints.end(), facePoints.begin(), facePoints.end());
        prototypeVertexCounts.push_back(facePoints.size());
    }

    const Offset prototypeVertexCount = prototypeFacePoints.size();
    std::vector<Offset> facePoints(copyCount * prototypeVertexCount);
    forEachCopy(copyCount, [&](Offset copyIndex) {
        const Offset pointStart = copyIndex * prototypePointCount;
        const Offset copyStart = copyIndex * prototypeVertexCount;
        for (Offset vertexIndex = 0; vertexIndex < prototypeVertexCount; ++vertexIndex)
            facePoints[copyStart + vertexIndex] = pointStart + prototypeFacePoints[vertexIndex];
    });

    std::vector<Offset> vertexCounts;
    vertexCounts.reserve(copyCount * prototypeVertexCounts.size());
    for (Offset copyIndex = 0; copyIndex < copyCount; ++copyIndex)
    {
        vertexCounts
            .insert(vertexCounts.end(), prototypeVertexCounts.begin(), prototypeVertexCounts.end());
    }
    copies->addFaces(facePoints, vertexCounts);

    // Attributes
    repeatOwnerAttributes(prototype, *copies, attr::AttributeOwner::POINT, copyCount);
    repeatOwnerAttributes(prototype, *copies, attr::AttributeOwner::VERTEX, copyCount);
    repeatOwnerAttributes(prototype, *copies, attr::AttributeOwner::FACE, copyCount);
    repeatOwnerAttributes(prototype, *copies, attr::AttributeOwner::PRIMITIVE, 1);

    return copies;
}

void CopyToPoints::cook()
{
    if (!outputRequested(0)) return;

    NodePacket prototypePacket = cloneInputPacket(0);
    NodePacket pointPacket = cloneInputPacket(1);
    NodePacket outputPacket;

    // Collects a transform for each target point, or a primitive's transform when it has no points.
    std::vector<Transform> transforms;
    for (const Transform& transform : pointPacket.getTransforms(TransformClass::POINT_PRIORITY))
        transforms.push_back(transform);

    for (const geo::PrimPtr& prim : prototypePacket.getPrimitives())
    {
        if (prim->transformType() == TransformClass::NONE) continue;

        // Fast copy for meshes
        if (const auto mesh = std::dynamic_pointer_cast<const geo::Mesh>(prim))
        {
            outputPacket.attemptMerge(copyMeshToTransforms(*mesh, transforms));
            continue;
        }

        // Slower copy for every other primitive type
        // It's fine that it's slower since they're usually much less expensive to copy
        for (const Transform& transform : transforms)
        {
            geo::PrimPtr copy = prim->clone();
            copy->applyTransform(transform, TransformClass::POINT_PRIORITY);
            outputPacket.attemptMerge(copy);
        }
    }

    setOutputPacket(0, outputPacket);
}

} // namespace

ENZO_REGISTER_NODE(copyToPoints, CopyToPoints)
