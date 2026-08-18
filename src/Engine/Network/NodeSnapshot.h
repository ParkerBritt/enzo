#pragma once
#include "Engine/Core/Path.h"
#include "Engine/Core/Types.h"
#include "Engine/Serializer/ParameterSerializable.h"
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <string>
#include <vector>

namespace enzo::nt {
class Node;

/**
 * @brief Everything one node carries, held apart from the network so it can be rebuilt.
 *
 * A node's type, where it sits, and the full state of its parameters, taken at one moment.
 * Undo captures a snapshot before a node goes away and rebuilds the node from it later, and
 * a saved file is a list of these plus the wiring between them.
 *
 * @note Connections and the nodes living in a child scope are not part of a snapshot. Wiring
 * belongs to the graph, and children are nodes in their own right, each snapshotted on their
 * own.
 */
class NodeSnapshot
{
  public:
    /// @brief Returns a snapshot of the node as it stands.
    static NodeSnapshot capture(Node& node);

    /// @brief Rebuilds the node under the given id with the path, position, and parameters it had.
    /// @note The scope the path sits in has to exist already, so a node holding other nodes is
    /// restored before the nodes living inside it.
    void restore(NodeId nodeId) const;

    /// @brief Returns the path the node sat at, whose leaf is the node name.
    const Path& getPath() const { return path_; }

  private:
    friend class cereal::access;

    template <class Archive> void save(Archive& archive) const
    {
        archive(
            cereal::make_nvp("typeName", typeName_),
            cereal::make_nvp("path", path_.getString()),
            cereal::make_nvp("posX", position_.x()),
            cereal::make_nvp("posY", position_.y()),
            cereal::make_nvp("parameters", parameters_)
        );
    }

    template <class Archive> void load(Archive& archive)
    {
        std::string path;
        float posX = 0;
        float posY = 0;
        archive(
            cereal::make_nvp("typeName", typeName_),
            cereal::make_nvp("path", path),
            cereal::make_nvp("posX", posX),
            cereal::make_nvp("posY", posY),
            cereal::make_nvp("parameters", parameters_)
        );
        path_ = Path(path);
        position_ = {posX, posY};
    }

    std::string typeName_;
    Path path_;
    Vector2 position_ = {0.f, 0.f};
    std::vector<ParameterSerializable> parameters_;
};
} // namespace enzo::nt
