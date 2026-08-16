#pragma once
#include "Engine/Core/Types.h"
#include <boost/functional/hash.hpp>
#include <cstddef>
#include <functional>
#include <string>

namespace enzo::nt {

/**
 * @brief A point in the network graph, either a node output or a parameter.
 *
 * An empty parm names the node's cooked output itself, while a filled parm
 * narrows the unit to one parameter component within the node.
 *
 * Example
 * Node 7's output is {7}, and the y of its translate is {7, "translate", 1}.
 */
struct Unit
{
    NodeId nodeId = 0;
    std::string parm;
    unsigned int index = 0;

    /// @brief Whether the unit is a whole node rather than one parameter.
    bool isNode() const { return parm.empty(); }

    /// @brief Whether the unit is one parameter rather than a whole node.
    bool isParameter() const { return !parm.empty(); }

    bool operator==(const Unit& other) const = default;
};

} // namespace enzo::nt

// Hashing lives in one place so a Unit can key an unordered container.
template <> struct std::hash<enzo::nt::Unit>
{
    std::size_t operator()(const enzo::nt::Unit& unit) const noexcept
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, unit.nodeId);
        boost::hash_combine(seed, unit.parm);
        boost::hash_combine(seed, unit.index);
        return seed;
    }
};
