#pragma once

#include "geometry/point2d.hpp"

#include <cstdint>

#define POINT_COORD_BITS 30

uint32_t DoubleToUint32(double x, double min, double max, uint32_t coordBits);

double Uint32ToDouble(uint32_t x, double min, double max, uint32_t coordBits);

m2::PointU PointDToPointU(double x, double y, uint32_t coordBits);

m2::PointU PointDToPointU(m2::PointD const & pt, uint32_t coordBits);

m2::PointD PointUToPointD(m2::PointU const & p, uint32_t coordBits);
