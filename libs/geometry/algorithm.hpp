#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"

#include <vector>

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

  PointD GetResult() const;

private:
  struct Value
  {
    Value(PointD const & p, double l) : m_p(p), m_len(l) {}
    bool operator<(Value const & r) const { return (m_len < r.m_len); }
    PointD m_p;
    double m_len;
  };

  std::vector<Value> m_poly;
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
  PointD GetResult() const { return m_center; }

private:
  PointD m_rectCenter;
  PointD m_center;
  double m_squareDistanceToApproximate;
};

// Calculates the smallest rect that includes given geometry.
class CalculateBoundingBox
{
public:
  void operator()(PointD const & p) { m_boundingBox.Add(p); }
  RectD GetResult() const { return m_boundingBox; }

private:
  RectD m_boundingBox;
};

template <typename TCalculator, typename TCollection>
auto ApplyCalculatorTrg(TCollection const & collection, TCalculator && calc)
{
  ASSERT(collection.size() % 3 == 0, ());
  for (size_t i = 0; i < collection.size(); i += 3)
    calc(collection[i], collection[i + 1], collection[i + 2]);
  return calc.GetResult();
}

template <typename TCalculator, typename TCollection>
auto ApplyCalculatorPoly(TCollection const & collection, TCalculator && calc)
{
  for (auto const & p : collection)
    calc(p);
  return calc.GetResult();
}

/// Returns the linearly-interpolated point at |targetDistance| along the polyline formed by
/// |points|, given pre-computed cumulative |distances| where distances[i] is the cumulative
/// length from points[0] to points[i + 1]. For |targetDistance| outside [0, distances.back()]
/// the corresponding endpoint is returned.
/// REQUIRES: distances.size() + 1 == points.size() and |distances| is non-decreasing.
PointD InterpolatePointAtDistance(std::vector<double> const & distances, std::vector<PointD> const & points,
                                  double targetDistance);

}  // namespace m2
