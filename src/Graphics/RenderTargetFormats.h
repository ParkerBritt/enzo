#pragma once
#include <GraphicsTypes.h>

namespace enzo::gfx {

/// @brief The format of the colour target every pass draws into.
inline constexpr Diligent::TEXTURE_FORMAT colorFormat = Diligent::TEX_FORMAT_RGBA8_UNORM;

/// @brief The format of the depth target every pass tests against.
inline constexpr Diligent::TEXTURE_FORMAT depthFormat = Diligent::TEX_FORMAT_D32_FLOAT;

/// @brief The sample count of the targets every pass draws into.
inline constexpr Diligent::Uint8 sampleCount = 4;

} // namespace enzo::gfx
