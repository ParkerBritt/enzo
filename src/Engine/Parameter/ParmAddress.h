#pragma once
#include "Engine/Core/Types.h"
#include <boost/functional/hash.hpp>
#include <cstddef>
#include <functional>
#include <string>

namespace enzo::prm {

/**
 * @brief Address of a single parameter component within the network.
 *
 * Where an nt::OpId points at a whole node, a ParmAddress narrows that down to
 * one value inside it by naming the node, the parameter, and the component
 * index. It is the stable key the dependency graph and the evaluation tracker
 * use to refer to a parameter.
 *
 * Example
 * The y component of node 7's "translate" parameter is {7, "translate", 1}.
 */
struct ParmAddress
{
    nt::OpId opId = 0;
    std::string parm;
    unsigned int index = 0;

    bool operator==(const ParmAddress& other) const = default;
};

} // namespace enzo::prm

// Hashing lives in one place so a ParmAddress can key an unordered container.
template <> struct std::hash<enzo::prm::ParmAddress>
{
    std::size_t operator()(const enzo::prm::ParmAddress& address) const noexcept
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, address.opId);
        boost::hash_combine(seed, address.parm);
        boost::hash_combine(seed, address.index);
        return seed;
    }
};
