#pragma once

#include "indexer/mercator.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"

namespace routing
{
class RouteFollower
{
public:
  RouteFollower() {}
  template <class IterT>
  RouteFollower(IterT begin, IterT end)
    : m_poly(begin, end)
  {
    Update();
  }

  void Swap(RouteFollower & rhs);

  struct IterT
  {
    m2::PointD m_pt;
    size_t m_ind;

    IterT(m2::PointD pt, size_t ind) : m_pt(pt), m_ind(ind) {}
    IterT() : m_ind(-1) {}

    bool IsValid() const { return m_ind != -1; }
  };

  IterT GetCurrentIter() { return m_current; }

  IterT FindProjection(m2::RectD const & posRect, double predictDistance = -1.0) const;

  void GetCurrentDirectionPoint(m2::PointD & pt) const;

private:
  template <class DistanceF>
  IterT GetClosestProjection(m2::RectD const & posRect, DistanceF const & distFn) const;

  void Update();

  double GetDistanceOnPolyline(IterT const & it1, IterT const & it2) const;

  m2::PolylineD m_poly;

  mutable IterT m_current;
  /// Precalculated info for fast projection finding.
  vector<m2::ProjectionToSection<m2::PointD>> m_segProj;
  /// Accumulated cache of segments length in meters.
  vector<double> m_segDistance;
};

}  // namespace routing
