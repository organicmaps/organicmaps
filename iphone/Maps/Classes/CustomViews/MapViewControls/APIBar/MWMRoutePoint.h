#pragma once

class MWMRoutePoint
{
public:
  MWMRoutePoint() {}

  MWMRoutePoint(m2::PointD const & p, NSString * n) : m_point(p), m_name(n), m_isMyPosition(false) {}

  explicit MWMRoutePoint(m2::PointD const & p) : m_point(p), m_name(L(@"p2p_your_location")), m_isMyPosition(true) {}

  bool operator ==(MWMRoutePoint const & p) const
  {
    return m_point.EqualDxDy(p.m_point, 0.00000001) && [m_name isEqualToString:p.m_name] && m_isMyPosition == p.m_isMyPosition;
  }

  bool operator !=(MWMRoutePoint const & p) const
  {
    return !(*this == p);
  }

  static MWMRoutePoint MWMRoutePointZero()
  {
    return MWMRoutePoint(m2::PointD::Zero(), @"");
  }

  m2::PointD const & Point() const
  {
    return m_point;
  }

  NSString * Name() const
  {
    return m_name;
  }

  bool IsMyPosition() const
  {
    return m_isMyPosition;
  }

private:
  m2::PointD m_point;
  NSString * m_name;
  bool m_isMyPosition;
};
