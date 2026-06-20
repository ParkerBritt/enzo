#pragma once
#include "Engine/Core/Types.h"
#include <boost/functional/hash.hpp>
#include <cstddef>
#include <functional>

namespace enzo::nt {

/**
 * @brief One wired link between two nodes, the ground truth of the network's wiring.
 *
 * Data flows from the source node's output slot into the target node's input slot,
 * so the target depends on the source.
 *
 * Example
 * Node 1's first output feeding node 2's second input is
 * {1, 0, 2, 1}.
 */
struct Connection
{
    OpId sourceOp = 0;
    unsigned int sourceOutput = 0;
    OpId targetOp = 0;
    unsigned int targetInput = 0;

    bool operator==(const Connection& other) const = default;
};

} // namespace enzo::nt

// Hashing lives in one place so a Connection can key an unordered container.
template <> struct std::hash<enzo::nt::Connection>
{
    std::size_t operator()(const enzo::nt::Connection& connection) const noexcept
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, connection.sourceOp);
        boost::hash_combine(seed, connection.sourceOutput);
        boost::hash_combine(seed, connection.targetOp);
        boost::hash_combine(seed, connection.targetInput);
        return seed;
    }
};
