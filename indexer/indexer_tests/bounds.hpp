#include "geometry/rect2d.hpp"

template <int MinX, int MinY, int MaxX, int MaxY>
struct Bounds
{
  enum
  {
    kMinX = MinX,
    kMaxX = MaxX,
    kMinY = MinY,
    kMaxY = MaxY,
    kRangeX = kMaxX - kMinX,
    kRangeY = kMaxY - kMinY,
  };

  static m2::RectD FullRect() { return {MinX, MinY, MaxX, MaxY}; }
};

using OrthoBounds = Bounds<-180, -90, 180, 90>;
