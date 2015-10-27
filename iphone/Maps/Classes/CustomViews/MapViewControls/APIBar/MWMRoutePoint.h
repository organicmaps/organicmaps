#pragma once

typedef struct MWMRoutePoint
{
  MWMRoutePoint() {}

  MWMRoutePoint(m2::PointD const & p, NSString * n) : point(p), name(n), isMyPosition(false) {}

  explicit MWMRoutePoint(m2::PointD const & p) : point(p), name(L(@"my_position")), isMyPosition(true) {}

  bool operator ==(MWMRoutePoint const & p) const
  {
    return point.EqualDxDy(p.point, 0.00000001) && [name isEqualToString:p.name] && isMyPosition == p.isMyPosition;
  }

  bool operator !=(MWMRoutePoint const & p) const
  {
    return !(*this == p);
  }

  static MWMRoutePoint MWMRoutePointZero()
  {
    return MWMRoutePoint(m2::PointD::Zero(), @"");
  }

  m2::PointD point;
  NSString * name;
  bool isMyPosition;

} MWMRoutePoint;
