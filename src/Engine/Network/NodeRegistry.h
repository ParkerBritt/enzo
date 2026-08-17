#pragma once
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeType.h"
#include <boost/config.hpp>
#include <string>

namespace enzo::nt {

/// @brief The prefix on the symbol a node library exports for each constructor.
inline constexpr const char* kNodeConstructorPrefix = "enzoNode_";

/// @brief Returns the exported symbol backing a constructor named in a manifest.
inline std::string nodeConstructorSymbol(const std::string& constructorName)
{
    return kNodeConstructorPrefix + constructorName;
}

} // namespace enzo::nt

/**
 * @brief Exposes a node implementation under the name its manifest asks for.
 *
 * Written once at the bottom of a node's source file. The name is an
 * identifier rather than a string and must match the manifest's
 * implementation constructor, which defaults to the node's own name.
 *
 * ```
 * ENZO_REGISTER_NODE(circle, Circle)
 * ```
 */
#define ENZO_REGISTER_NODE(constructorName, ImplementationClass)                                   \
    extern "C" BOOST_SYMBOL_EXPORT enzo::nt::NodeImpl* enzoNode_##constructorName(                 \
        enzo::nt::Node& node,                                                                      \
        enzo::nt::CookContext& context                                                             \
    )                                                                                              \
    {                                                                                              \
        return new ImplementationClass(node, context);                                             \
    }
