#pragma once

#include "geometry/mercator.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstddef>
#include <limits>
#include <vector>

namespace routing
{
class FollowedPolyline
{
public:
  FollowedPolyline() = default;

  template <typename Iter>
  FollowedPolyline(Iter begin, Iter end) : m_poly(begin, end)
  {
    Update();
    // Initially we do not have intermediate points. Next checkpoint is finish.
    m_nextCheckpointIndex = m_segProj.size();
  }

  void SetNextCheckpointIndex(size_t index) { m_nextCheckpointIndex = index; }

  void Swap(FollowedPolyline & rhs);

  void Append(FollowedPolyline const & poly)
  {
    m_poly.Append(poly.m_poly);
    Update();
  }

  void PopBack()
  {
    m_poly.PopBack();
    Update();
  }

  bool IsValid() const { return (m_current.IsValid() && m_poly.GetSize() > 1); }
  m2::PolylineD const & GetPolyline() const { return m_poly; }

  std::vector<double> const & GetSegDistanceMeters() const { return m_segDistance; }
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
    static size_t constexpr kInvalidIndex = std::numeric_limits<size_t>::max();

    Iter() = default;
    Iter(m2::PointD pt, size_t ind) : m_pt(pt), m_ind(ind) {}

    bool IsValid() const { return m_ind != kInvalidIndex; }

    m2::PointD m_pt;
    size_t m_ind = kInvalidIndex;
  };

  Iter GetCurrentIter() const { return m_current; }

  double GetDistanceM(Iter const & it1, Iter const & it2) const;

  /// \brief Sets indexes of all unmatching segments on route.
  void SetFakeSegmentIndexes(std::vector<size_t> && fakeSegmentIndexes);

  /// \brief Updates projection to the closest matched segment if it's possible.
  bool UpdateMatchingProjection(m2::RectD const & posRect);

  Iter UpdateProjection(m2::RectD const & posRect);

  Iter Begin() const;
  Iter End() const;
  Iter GetIterToIndex(size_t index) const;

  /// \brief Calculates projection of center of |posRect| to the polyline.
  /// \param posRect Only projection inside the rect is considered.
  /// \param distFn A method which is used to calculate the destination between points.
  /// \param startIdx Start segment index in |m_segProj|.
  /// \param endIdx The index after the last one in |m_segProj|.
  /// \returns iterator which contains projection point and projection segment index.
  template <typename DistanceFn>
  Iter GetClosestProjectionInInterval(m2::RectD const & posRect, DistanceFn const & distFn, size_t startIdx,
                                      size_t endIdx) const
  {
    CHECK_LESS_OR_EQUAL(endIdx, m_segProj.size(), ());
    CHECK_LESS_OR_EQUAL(startIdx, endIdx, ());

    Iter res;
    double minDist = std::numeric_limits<double>::max();

    for (size_t i = startIdx; i < endIdx; ++i)
    {
      m2::PointD const & pt = m_segProj[i].ClosestPointTo(posRect.Center());

      if (!posRect.IsPointInside(pt))
        continue;

      Iter it(pt, i);
      double const dp = distFn(it);
      if (dp < minDist)
      {
        res = it;
        minDist = dp;
      }
    }

    return res;
  }

  Iter GetClosestMatchingProjectionInInterval(m2::RectD const & posRect, size_t startIdx, size_t endIdx) const;

  bool IsFakeSegment(size_t index) const;

private:
  /// \returns iterator to the best projection of center of |posRect| to the |m_poly|.
  /// If there's a good projection of center of |posRect| to two closest segments of |m_poly|
  /// after |m_current| the iterator corresponding of the projection is returned.
  /// Otherwise returns a projection to closest point of route.
  template <typename DistanceFn>
  Iter GetBestProjection(m2::RectD const & posRect, DistanceFn const & distFn) const;

  Iter GetBestMatchingProjection(m2::RectD const & posRect) const;

  void Update();

  m2::PolylineD m_poly;
  /// Indexes of all unmatching segments on route.
  std::vector<size_t> m_fakeSegmentIndexes;

  /// Iterator with the current position. Position sets with UpdateProjection methods.
  Iter m_current;
  size_t m_nextCheckpointIndex;
  /// Precalculated info for fast projection finding.
  std::vector<m2::ParametrizedSegment<m2::PointD>> m_segProj;
  /// Accumulated cache of segments length in meters.
  std::vector<double> m_segDistance;
};
}  // namespace routing
