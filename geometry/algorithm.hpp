#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "std/vector.hpp"

namespace m2
{
// This class is used to calculate a point on a polyline
// such as that the paths (not the distance) from that point
// to both ends of a polyline have the same length.
class CalculatePolyLineCenter
{
public:
  CalculatePolyLineCenter() : m_length(0.0) {}
  void operator()(PointD const & pt);

  PointD GetCenter() const;

private:
  struct Value
  {
    Value(PointD const & p, double l) : m_p(p), m_len(l) {}
    bool operator<(Value const & r) const { return (m_len < r.m_len); }
    PointD m_p;
    double m_len;
  };

  vector<Value> m_poly;
  double m_length;
};

// This class is used to calculate a point such as that
// it lays on the figure triangle that is the closest to
// figure bounding box center.
class CalculatePointOnSurface
{
public:
  CalculatePointOnSurface(RectD const & rect);

  void operator()(PointD const & p1, PointD const & p2, PointD const & p3);
  PointD GetCenter() const { return m_center; }
private:
  PointD m_rectCenter;
  PointD m_center;
  double m_squareDistanceToApproximate;
};

// Calculates the smallest rect that includes given geometry.
class CalculateBoundingBox
{
public:
  void operator()(PointD const & p);
  RectD GetBoundingBox() const { return m_boundingBox; }
private:
  RectD m_boundingBox;
};
}  // namespace m2
