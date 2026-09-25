#pragma once
#include "Engine/Core/Types.h"
#include <boost/functional/hash.hpp>
#include <cstddef>
#include <functional>

namespace enzo::nt {

/**
 * @brief One wired link between two nodes, the ground truth of the network's wiring.
 *
 * Data flows from the source node's output into the target node's input,
 * so the target depends on the source.
 *
 * Example
 * Node 1's first output feeding node 2's second input is
 * {1, 0, 2, 1}.
 */
struct NodeLink
{
    NodeId sourceNode = 0;
    unsigned int sourceOutput = 0;
    NodeId targetNode = 0;
    unsigned int targetInput = 0;

    bool operator==(const NodeLink& other) const = default;
};

} // namespace enzo::nt

// Hashing lives in one place so a Node link can key an unordered container.
template <> struct std::hash<enzo::nt::NodeLink>
{
    std::size_t operator()(const enzo::nt::NodeLink& nodeLink) const noexcept
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, nodeLink.sourceNode);
        boost::hash_combine(seed, nodeLink.sourceOutput);
        boost::hash_combine(seed, nodeLink.targetNode);
        boost::hash_combine(seed, nodeLink.targetInput);
        return seed;
    }
};
