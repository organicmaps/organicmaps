#pragma once
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/bits.hpp"
#include "point2d.hpp"

namespace m2
{

inline PointU Uint64ToPointU(int64_t v)
{
  uint32_t const hi = bits::PerfectUnshuffle(static_cast<uint32_t>(v >> 32));
  uint32_t const lo = bits::PerfectUnshuffle(static_cast<uint32_t>(v & 0xFFFFFFFFULL));
  return PointU((((hi & 0xFFFF) << 16) | (lo & 0xFFFF)), ((hi & 0xFFFF0000) | (lo >> 16)));
}

inline uint64_t PointUToUint64(PointU const & pt)
{
  uint64_t const res =
      (static_cast<int64_t>(bits::PerfectShuffle((pt.y & 0xFFFF0000) | (pt.x >> 16))) << 32) |
      bits::PerfectShuffle(((pt.y & 0xFFFF) << 16 ) | (pt.x & 0xFFFF));
  ASSERT_EQUAL(pt, Uint64ToPointU(res), ());
  return res;
}

} // namespace m2
