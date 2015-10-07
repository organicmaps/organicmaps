#pragma once

typedef struct MWMRoutePoint
{
  MWMRoutePoint() {};

  MWMRoutePoint(m2::PointD const & p, NSString * n) : point(p), name(n), isMyPosition(false) {};

  MWMRoutePoint(m2::PointD const & p) : point(p), name(L(@"my_position")), isMyPosition(true) {};

  bool operator ==(MWMRoutePoint const & p) const
  {
    return point == p.point && [name isEqualToString:p.name];
  }

  bool operator !=(MWMRoutePoint const & p) const
  {
    return !(*this == p);
  }

  static MWMRoutePoint MWMRoutePointZero()
  {
    return MWMRoutePoint(m2::PointD::Zero(), @"");
  }

  static void Swap(MWMRoutePoint & f, MWMRoutePoint & s)
  {
    swap(f.point, s.point);
    NSString * temp = f.name;
    f.name = s.name;
    s.name = temp;
  }

  m2::PointD point;
  NSString * name;
  bool isMyPosition;

} MWMRoutePoint;
