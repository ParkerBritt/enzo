#pragma once

namespace enzo::attr {

/**
 * @brief Names of the attributes the engine gives a meaning to.
 */
namespace names {

/// The point position every mesh is created with.
inline constexpr const char* position = "P";
/// The direction a face, point or vertex faces.
inline constexpr const char* normal = "Normal";
/// The direction that orients a point's up axis.
inline constexpr const char* up = "Up";

} // namespace names

} // namespace enzo::attr
