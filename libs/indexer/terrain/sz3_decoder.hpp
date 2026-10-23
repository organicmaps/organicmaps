#pragma once

#include "geometry/point_with_altitude.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace terrain
{
// Decodes a RasterCodec::Sz3 frame (see sz3_profile.hpp) of nodesX * nodesY heights, clamped to
// the source range and rounded like DecodeSperr.
std::vector<geometry::Altitude> DecodeSz3(std::span<uint8_t const> frame, uint32_t nodesX, uint32_t nodesY,
                                          geometry::Altitude minimum, geometry::Altitude maximum);
}  // namespace terrain
