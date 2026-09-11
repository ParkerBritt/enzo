#include "Engine/Network/NodePacket.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Primitives/Mesh.h"
#include <cstddef>
#include <span>
#include <stdexcept>

namespace enzo {

namespace {

const std::string kNormalAttribute = "Normal";
const std::string kUpAttribute = "Up";

/**
 * @brief Returns whether an attribute exists and holds vector values.
 */
bool isPopulatedVector(const std::shared_ptr<attr::Attribute>& attribute)
{
    return attribute && attribute->getType() == attr::AttrType::vectorT &&
           attribute->getSize() > 0;
}

/**
 * @brief Returns the vector each point carries, averaging the vertices that meet there.
 *
 * @note A point no vertex refers to comes back as a zero vector.
 */
std::vector<Vector3> getPointVectorsFromVertices(
    geo::Mesh& mesh,
    const std::shared_ptr<attr::Attribute>& vertexAttribute
)
{
    const attr::AttributeHandle<Vector3> vertexValues(vertexAttribute);
    const std::span<const intT> vertexPoints = mesh.vertexPointSpan();

    std::vector<Vector3> pointValues(mesh.getNumPoints(), Vector3::Zero());
    for (Offset vertexOffset = 0; vertexOffset < vertexPoints.size(); ++vertexOffset)
    {
        if (!mesh.isValidVertex(vertexOffset)) continue;
        pointValues[vertexPoints[vertexOffset]] += vertexValues[vertexOffset];
    }

    for (Vector3& value : pointValues)
    {
        if (value.squaredNorm() > 0) value.normalize();
    }

    return pointValues;
}

/**
 * @brief Returns the named attribute as one vector per point, preferring vertex over point.
 *
 * @return A vector for every point, or an empty vector when the primitive carries neither.
 */
std::vector<Vector3> getPointVectors(geo::Primitive& prim, const std::string& name)
{
    if (auto* mesh = dynamic_cast<geo::Mesh*>(&prim))
    {
        std::shared_ptr<attr::Attribute> vertexAttribute =
            mesh->getAttribByName(attr::AttributeOwner::VERTEX, name);
        if (isPopulatedVector(vertexAttribute))
            return getPointVectorsFromVertices(*mesh, vertexAttribute);
    }

    std::shared_ptr<attr::Attribute> pointAttribute =
        prim.getAttribByName(attr::AttributeOwner::POINT, name);
    if (!isPopulatedVector(pointAttribute)) return {};

    return attr::AttributeHandle<Vector3>(pointAttribute).getAllValues();
}

/**
 * @brief Returns the rotation that aims each point down its normal, with Up taking the roll.
 *
 * @return One rotation per point, or an empty vector when the primitive has no normals.
 *
 * @note Points with no normal come back as the identity.
 */
std::vector<Transform> getPointOrientations(geo::Primitive& prim)
{
    const std::vector<Vector3> normals = getPointVectors(prim, kNormalAttribute);
    if (normals.empty()) return {};

    const std::vector<Vector3> ups = getPointVectors(prim, kUpAttribute);

    std::vector<Transform> orientations(normals.size());
    for (Offset pointOffset = 0; pointOffset < normals.size(); ++pointOffset)
    {
        const Vector3& normal = normals[pointOffset];
        if (normal.squaredNorm() <= 0) continue;

        Vector3 up = Vector3::UnitY();
        const bool hasUp = pointOffset < ups.size() && ups[pointOffset].squaredNorm() > 0;
        if (hasUp) up = ups[pointOffset];

        orientations[pointOffset] = Transform::lookAt(Vector3::Zero(), normal, up);
    }

    return orientations;
}

} // namespace

// ---
// Transforms::Iterator
// ---

NodePacket::Transforms::Iterator::Iterator(
    std::vector<std::shared_ptr<geo::Primitive>>& primitives,
    TransformClass transformClass,
    size_t primIdx
)
    : primitives_(primitives), transformClass_(transformClass), primIdx_(primIdx)
{
    advance();
}

Transform NodePacket::Transforms::Iterator::operator*() const
{
    Transform transform = Transform::fromAttribute(*curAttrib_, offset_);
    if (offset_ < orientations_.size()) transform.compose(orientations_[offset_]);
    return transform;
}

NodePacket::Transforms::Iterator& NodePacket::Transforms::Iterator::operator++()
{
    ++offset_;
    if (offset_ >= curSize_)
    {
        ++primIdx_;
        offset_ = 0;
        advance();
    }
    return *this;
}

NodePacket::Transforms::Iterator NodePacket::Transforms::Iterator::operator++(int)
{
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

void NodePacket::Transforms::Iterator::advance()
{
    curAttrib_ = nullptr;
    curSize_ = 0;
    orientations_.clear();
    while (primIdx_ < primitives_.size())
    {
        auto& prim = primitives_[primIdx_];
        TransformClass primTransformClass = prim->transformType();

        // POINT takes priority: use P attribute if both the query and primitive support it
        if (hasFlag(transformClass_, TransformClass::POINT) &&
            hasFlag(primTransformClass, TransformClass::POINT))
        {
            auto attrib = prim->getAttribByName(attr::AttributeOwner::POINT, "P", true);
            if (attrib && attrib->getSize() > 0)
            {
                curAttrib_ = attrib;
                curSize_ = attrib->getSize();
                orientations_ = getPointOrientations(*prim);
                return;
            }
        }

        // Fallback: primitive-level transform attribute
        if (hasFlag(transformClass_, TransformClass::PRIMITIVE) &&
            hasFlag(primTransformClass, TransformClass::PRIMITIVE))
        {
            auto attrib = prim->getAttribByName(attr::AttributeOwner::PRIMITIVE, "transform", true);
            if (attrib && attrib->getSize() > 0)
            {
                curAttrib_ = attrib;
                curSize_ = attrib->getSize();
                return;
            }
        }

        ++primIdx_;
    }
}

// ---
// NodePacket
// ---

void NodePacket::addPrimitive(std::shared_ptr<geo::Primitive> primitive)
{
    primitives_.push_back(std::move(primitive));
}

void NodePacket::attemptMerge(std::shared_ptr<geo::Primitive> newPrim)
{
    if (newPrim->canMerge())
    {
        auto existing = getPrimAtPath(newPrim->getPath());
        if (existing)
        {
            existing->merge(newPrim);
            return;
        }
    }
    else if (getPrimAtPath(newPrim->getPath()))
    {
        newPrim->incrementVersion();
    }
    addPrimitive(newPrim);
}

std::shared_ptr<geo::Primitive> NodePacket::getPrimitive(unsigned int index)
{
    if (index >= primitives_.size())
        throw std::out_of_range("NodePacket::getPrimitive index out of range");
    return primitives_.at(index);
}

std::shared_ptr<const geo::Primitive> NodePacket::getPrimitive(unsigned int index) const
{
    if (index >= primitives_.size())
        throw std::out_of_range("NodePacket::getPrimitive index out of range");
    return primitives_.at(index);
}

std::shared_ptr<geo::Primitive> NodePacket::getPrimAtPath(const std::string& path)
{
    for (auto& prim : primitives_)
    {
        if (prim->getPath() == path) return prim;
    }
    return nullptr;
}

std::shared_ptr<const geo::Primitive> NodePacket::getPrimAtPath(const std::string& path) const
{
    for (const auto& prim : primitives_)
    {
        if (prim->getPath() == path) return prim;
    }
    return nullptr;
}

std::vector<std::shared_ptr<geo::Primitive>> NodePacket::getPrimitives(geo::PrimType type) const
{
    std::vector<std::shared_ptr<geo::Primitive>> out;
    for (const auto& prim : primitives_)
    {
        if (prim->getType() == type) out.push_back(prim);
    }
    return out;
}

size_t NodePacket::size() const { return primitives_.size(); }

NodePacket NodePacket::deepCopy() const
{
    NodePacket copy;
    for (const auto& prim : primitives_)
        copy.addPrimitive(prim->clone());
    return copy;
}

// TODO: remplace with PrimPath
void NodePacket::removePrim(std::string path)
{
    for (auto it = primitives_.begin(); it != primitives_.end(); it++)
    {
        if ((*it)->getPath() == path)
        {
            primitives_.erase(it);
            return;
        }
    }
}

} // namespace enzo
