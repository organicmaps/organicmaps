#pragma once

#include "indexer/mercator.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"

namespace routing
{
class FollowedPolyline
{
public:
  FollowedPolyline() {}
  template <class TIter>
  FollowedPolyline(TIter begin, TIter end)
    : m_poly(begin, end)
  {
    Update();
  }

  void Swap(FollowedPolyline & rhs);

  bool IsValid() const { return (m_current.IsValid() && m_poly.GetSize() > 0); }

  m2::PolylineD const & GetPolyline() const { return m_poly; }

  double GetMercatorDistanceFromBegin() const;

  /*! \brief Return next navigation point for direction widgets.
   *  Returns first geomety point from the polyline after your location if it is farther then
   *  toleranceM.
   */
  void GetCurrentDirectionPoint(m2::PointD & pt, double toleranceM) const;

  struct Iter
  {
    m2::PointD m_pt;
    size_t m_ind;

    Iter(m2::PointD pt, size_t ind) : m_pt(pt), m_ind(ind) {}
    Iter() : m_ind(-1) {}

    bool IsValid() const { return m_ind != -1; }
  };

  const Iter GetCurrentIter() const { return m_current; }

  double GetDistanceM(Iter const & it1, Iter const & it2) const;

  Iter UpdateProjectionByPrediction(m2::RectD const & posRect, double predictDistance) const;
  Iter UpdateProjection(m2::RectD const & posRect) const;

  Iter Begin() const;
  Iter End() const;
  Iter GetIterToIndex(size_t index) const;

private:
  template <class DistanceFn>
  Iter GetClosestProjection(m2::RectD const & posRect, DistanceFn const & distFn) const;

  void Update();

  m2::PolylineD m_poly;

  /// Cached result iterator for last MoveIterator query.
  mutable Iter m_current;
  /// Precalculated info for fast projection finding.
  vector<m2::ProjectionToSection<m2::PointD>> m_segProj;
  /// Accumulated cache of segments length in meters.
  vector<double> m_segDistance;
};

}  // namespace routing
