#pragma once
#include "Engine/Core/Path.h"
#include "Engine/Core/Types.h"
#include "Engine/Serializer/ParameterSerializable.h"
#include <string>
#include <vector>

namespace enzo::nt {
/**
 * @brief Everything one node carries, held apart from the network so it can be rebuilt.
 *
 * A node's type, where it sits, and the full state of its parameters, taken at one moment.
 * Undo captures a snapshot before a node goes away and rebuilds the node from it later.
 *
 * @note Connections and the nodes living in a child scope are not part of a snapshot. Wiring
 * belongs to the graph, and children are nodes in their own right, each restored by its own
 * undo command.
 */
class NodeSnapshot
{
  public:
    /// @brief Returns a snapshot of the node as it stands.
    static NodeSnapshot capture(NodeId nodeId);

    /// @brief Rebuilds the node under the given id with the path, position, and parameters it had.
    void restore(NodeId nodeId) const;

  private:
    std::string typeName_;
    Path path_;
    Vector2 position_ = {0.f, 0.f};
    std::vector<ParameterSerializable> parameters_;
};
} // namespace enzo::nt
