#pragma once

#include "geometry/point_with_altitude.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace terrain
{
char constexpr kSperrRevision[] = "b8012586908aaabba993038d183b3937c8ea9c7c";

std::vector<geometry::Altitude> DecodeSperr(std::span<uint8_t const> frame, uint32_t nodesX, uint32_t nodesY,
                                            geometry::Altitude minimum, geometry::Altitude maximum);
}  // namespace terrain
