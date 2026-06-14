#pragma once

#include <string_view>

namespace dp
{
std::string_view constexpr k45dHatching = "45d";
std::string_view constexpr kDashHatching = "dash";

// Solid-fill area patterns (single pass; modulate the surface colour instead of masking it).
std::string_view constexpr kStipplePattern = "stipple";
std::string_view constexpr kSpecklePattern = "speckle";
std::string_view constexpr kGridPattern = "grid";
}  // namespace dp
