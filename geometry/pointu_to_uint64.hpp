#pragma once
#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/bits.hpp"
#include "geometry/point2d.hpp"

namespace m2
{

inline PointU Uint64ToPointU(int64_t v)
{
  PointU res;
  bits::BitwiseSplit(v, res.x, res.y);
  return res;
}

inline uint64_t PointUToUint64(PointU const & pt)
{
  uint64_t const res = bits::BitwiseMerge(pt.x, pt.y);
  ASSERT_EQUAL(pt, Uint64ToPointU(res), ());
  return res;
}

} // namespace m2
