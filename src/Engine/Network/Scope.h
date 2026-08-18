#pragma once
#include "Engine/Core/Path.h"
#include <string>

namespace enzo::nt {
/**
 * @brief One place nodes live, either the root or the inside of a node that holds one.
 *
 * A scope is the region a node name has to be unique within and the place a relative
 * path is read from. Nodes are not stored in it. They sit in the nt::Network's store
 * and say which scope they are in through their path, so a scope is what that path
 * names.
 *
 * Example
 *
 * ```
 * /                    the root scope
 * /container1          a node holding a scope
 * /container1/grid1    a node living inside that scope
 * ```
 *
 * @note The type decides what may be created inside. A "geometry" scope takes geometry
 * nodes, and the root scope is the one the scene opens on.
 */
class Scope
{
  public:
    Scope(const Path& path, std::string scopeType) : path_{path}, scopeType_{std::move(scopeType)}
    {
    }

    /// @brief Returns the path this scope sits at, which its nodes are children of.
    const Path& getPath() const { return path_; }

    /// @brief Returns the kind of nodes this scope holds, such as "geometry".
    const std::string& getType() const { return scopeType_; }

  private:
    Path path_;
    std::string scopeType_;
};
} // namespace enzo::nt
