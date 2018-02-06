#pragma once

#include "geometry/cellid.hpp"
#include "geometry/rect2d.hpp"

#include <utility>

#define POINT_COORD_BITS 30

m2::PointU PointD2PointU(double x, double y, uint32_t coordBits);
inline m2::PointU PointD2PointU(m2::PointD const & pt, uint32_t coordBits)
{
  return PointD2PointU(pt.x, pt.y, coordBits);
}

m2::PointD PointU2PointD(m2::PointU const & p, uint32_t coordBits);

int64_t PointToInt64(double x, double y, uint32_t coordBits);
inline int64_t PointToInt64(m2::PointD const & pt, uint32_t coordBits)
{
  return PointToInt64(pt.x, pt.y, coordBits);
}

m2::PointD Int64ToPoint(int64_t v, uint32_t coordBits);

std::pair<int64_t, int64_t> RectToInt64(m2::RectD const & r, uint32_t coordBits);
m2::RectD Int64ToRect(std::pair<int64_t, int64_t> const & p, uint32_t coordBits);

uint32_t DoubleToUint32(double x, double min, double max, uint32_t coordBits);

double Uint32ToDouble(uint32_t x, double min, double max, uint32_t coordBits);
