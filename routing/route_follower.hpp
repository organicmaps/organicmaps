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
  template <class TIter>
  RouteFollower(TIter begin, TIter end)
    : m_poly(begin, end)
  {
    Update();
  }

  void Swap(RouteFollower & rhs);

  struct Iter
  {
    m2::PointD m_pt;
    size_t m_ind;

    Iter(m2::PointD pt, size_t ind) : m_pt(pt), m_ind(ind) {}
    Iter() : m_ind(-1) {}

    bool IsValid() const { return m_ind != -1; }
  };

  Iter GetCurrentIter() { return m_current; }

  Iter FindProjection(m2::RectD const & posRect, double predictDistance = -1.0) const;

  void GetCurrentDirectionPoint(m2::PointD & pt) const;

private:
  template <class DistanceFn>
  Iter GetClosestProjection(m2::RectD const & posRect, DistanceFn const & distFn) const;

  void Update();

  double GetDistanceOnPolyline(Iter const & it1, Iter const & it2) const;

  m2::PolylineD m_poly;

  mutable Iter m_current;
  /// Precalculated info for fast projection finding.
  vector<m2::ProjectionToSection<m2::PointD>> m_segProj;
  /// Accumulated cache of segments length in meters.
  vector<double> m_segDistance;
};

}  // namespace routing
