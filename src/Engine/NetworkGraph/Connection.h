#pragma once
#include "Engine/Core/Types.h"

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
