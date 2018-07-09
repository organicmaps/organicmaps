#pragma once

#include "scales.hpp"

#ifdef BUILD_DESIGNER

enum { kPatchScaleShift = 3 };

inline uint32_t PatchMinDrawableScale(uint32_t s)
{
  return (s < kPatchScaleShift ? 0 : s - kPatchScaleShift);
}

inline uint32_t PatchScaleBound(uint32_t s)
{
  s += kPatchScaleShift;
  if (s > scales::GetUpperScale())
    s = scales::GetUpperScale();
  return s;
}

#else // BUILD_DESIGNER

inline uint32_t PatchMinDrawableScale(uint32_t s) { return s; }

inline uint32_t PatchScaleBound(uint32_t s) { return s; }

#endif // BUILD_DESIGNER
