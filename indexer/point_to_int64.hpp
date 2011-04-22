#pragma once

#include "../geometry/cellid.hpp"
#include "../geometry/rect2d.hpp"

#include "../std/utility.hpp"

#define COORD_BITS 30

typedef double CoordT;
typedef pair<CoordT, CoordT> CoordPointT;

typedef m2::CellId<19> RectId;

m2::PointU PointD2PointU(CoordT x, CoordT y, uint32_t coordBits = COORD_BITS);
CoordPointT PointU2PointD(m2::PointU const & p, uint32_t coordBits = COORD_BITS);

int64_t PointToInt64(CoordT x, CoordT y, uint32_t coordBits = COORD_BITS);
inline int64_t PointToInt64(CoordPointT const & pt, uint32_t coordBits = COORD_BITS)
{
  return PointToInt64(pt.first, pt.second, coordBits);
}
CoordPointT Int64ToPoint(int64_t v, uint32_t coordBits = COORD_BITS);

pair<int64_t, int64_t> RectToInt64(m2::RectD const & r, uint32_t coordBits = COORD_BITS);
m2::RectD Int64ToRect(pair<int64_t, int64_t> const & p, uint32_t coordBits = COORD_BITS);
