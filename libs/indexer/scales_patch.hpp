#pragma once

#include "indexer/scales.hpp"

#include <algorithm>

namespace scales
{
// In Designer mode (StyleReader::IsDesignerMode()) drawable scale ranges are
// widened during map generation so edited features stay visible while their
// zoom ranges are being tuned. The flag is passed in so callers can hoist it
// out of per-feature loops and this header stays decoupled from StyleReader.
uint8_t constexpr kPatchScaleShift = 3;

inline uint32_t PatchMinDrawableScale(uint32_t s, bool designerMode)
{
  if (!designerMode)
    return s;
  return s < kPatchScaleShift ? 0 : s - kPatchScaleShift;
}

inline uint32_t PatchMaxDrawableScale(uint32_t s, bool designerMode)
{
  if (designerMode)
    return std::min(s + kPatchScaleShift, static_cast<uint32_t>(GetUpperStyleScale()));

  // Some features can start drawing after indexer's GetUpperScale(),
  // so extend upper bound for valid IsDrawableInRange check.
  return s == GetUpperScale() ? GetUpperStyleScale() : s;
}
}  // namespace scales
