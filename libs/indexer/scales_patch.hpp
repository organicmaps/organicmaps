#pragma once

#include "scales.hpp"

namespace scales
{
#ifdef BUILD_DESIGNER

uint8_t constexpr kPatchScaleShift = 3;

inline uint32_t PatchMinDrawableScale(uint32_t s)
{
  return (s < kPatchScaleShift ? 0 : s - kPatchScaleShift);
}

inline uint32_t PatchMaxDrawableScale(uint32_t s)
{
  std::min(s + kPatchScaleShift, static_cast<uint32_t>(GetUpperStyleScale()));
}

#else  // BUILD_DESIGNER

inline uint32_t PatchMinDrawableScale(uint32_t s)
{
  return s;
}

inline uint32_t PatchMaxDrawableScale(uint32_t s)
{
  // Some features can start drawing after indexer's GetUpperScale(),
  // so extend upper bound for valid IsDrawableInRange check.
  return (s == GetUpperScale()) ? GetUpperStyleScale() : s;
}

#endif  // BUILD_DESIGNER
}  // namespace scales
