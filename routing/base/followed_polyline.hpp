#pragma once

#include "geometry/mercator.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"

namespace routing
{
class FollowedPolyline
{
public:
  FollowedPolyline() = default;
  template <class TIter>
  FollowedPolyline(TIter begin, TIter end)
    : m_poly(begin, end)
  {
    Update();
  }

  void Swap(FollowedPolyline & rhs);

  bool IsValid() const { return (m_current.IsValid() && m_poly.GetSize() > 1); }
  m2::PolylineD const & GetPolyline() const { return m_poly; }
  vector<double> const & GetSegDistanceMeters() const { return m_segDistance; }
  double GetTotalDistanceMeters() const;
  double GetDistanceFromStartMeters() const;
  double GetDistanceToEndMeters() const;
  double GetDistFromCurPointToRoutePointMerc() const;
  double GetDistFromCurPointToRoutePointMeters() const;

  /*! \brief Return next navigation point for direction widgets.
   *  Returns first geometry point from the polyline after your location if it is farther then
   *  toleranceM.
   */
  void GetCurrentDirectionPoint(m2::PointD & pt, double toleranceM) const;

  struct Iter
  {
    m2::PointD m_pt;
    size_t m_ind;

    static size_t constexpr kInvalidIndex = std::numeric_limits<size_t>::max();

    Iter(m2::PointD pt, size_t ind) : m_pt(pt), m_ind(ind) {}
    Iter() : m_ind(kInvalidIndex) {}
    bool IsValid() const { return m_ind != kInvalidIndex; }
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
  Iter GetClosestProjectionInInterval(m2::RectD const & posRect, DistanceFn const & distFn,
                                      size_t startIdx, size_t endIdx) const;
  template <class DistanceFn>
  Iter GetClosestProjection(m2::RectD const & posRect, DistanceFn const & distFn) const;

  void Update();

  m2::PolylineD m_poly;

  /// Iterator with the current position. Position sets with UpdateProjection methods.
  mutable Iter m_current;
  /// Precalculated info for fast projection finding.
  vector<m2::ProjectionToSection<m2::PointD>> m_segProj;
  /// Accumulated cache of segments length in meters.
  vector<double> m_segDistance;
};

}  // namespace routing
