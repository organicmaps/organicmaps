#pragma once

#include "geometry/rect2d.hpp"

template <int MinX, int MinY, int MaxX, int MaxY>
struct Bounds
{
  static double constexpr kMinX = MinX;
  static double constexpr kMaxX = MaxX;
  static double constexpr kMinY = MinY;
  static double constexpr kMaxY = MaxY;
  static double constexpr kRangeX = kMaxX - kMinX;
  static double constexpr kRangeY = kMaxY - kMinY;

  static m2::RectD FullRect() { return {MinX, MinY, MaxX, MaxY}; }
};

using OrthoBounds = Bounds<-180, -90, 180, 90>;
