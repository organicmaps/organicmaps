#include "routing/nearest_edge_finder.hpp"

#include "geometry/distance.hpp"

#include "indexer/feature.hpp"

#include "base/assert.hpp"

#include "std/limits.hpp"

namespace routing
{
NearestEdgeFinder::Candidate::Candidate() : m_dist(numeric_limits<double>::max()), m_segId(0) {}
NearestEdgeFinder::NearestEdgeFinder(m2::PointD const & point)
    : m_point(point)
{
}

void NearestEdgeFinder::AddInformationSource(FeatureID const & featureId, IRoadGraph::RoadInfo const & roadInfo)
{
  Candidate res;

  size_t const count = roadInfo.m_junctions.size();
  ASSERT_GREATER(count, 1, ());
  for (size_t i = 1; i < count; ++i)
  {
    /// @todo Probably, we need to get exact projection distance in meters.
    m2::ProjectionToSection<m2::PointD> segProj;
    segProj.SetBounds(roadInfo.m_junctions[i - 1].GetPoint(), roadInfo.m_junctions[i].GetPoint());

    m2::PointD const pt = segProj(m_point);
    double const d = m_point.SquareLength(pt);
    if (d < res.m_dist)
    {
      res.m_dist = d;
      res.m_fid = featureId;
      res.m_segId = static_cast<uint32_t>(i - 1);
      res.m_segStart = roadInfo.m_junctions[i - 1];
      res.m_segEnd = roadInfo.m_junctions[i];
      // @TODO res.m_projPoint.GetAltitude() is an altitude of |pt|. |pt| is a projection of m_point
      // to segment [res.m_segStart.GetPoint(), res.m_segEnd.GetPoint()].
      // It's necessary to calculate exact value of res.m_projPoint.GetAltitude() by this
      // information.
      bool const isAltitude = res.m_segStart.GetAltitude() != feature::kInvalidAltitude &&
                              res.m_segEnd.GetAltitude() != feature::kInvalidAltitude;
      feature::TAltitude const projPointAlt =
          isAltitude ? static_cast<feature::TAltitude>(
                           (res.m_segStart.GetAltitude() + res.m_segEnd.GetAltitude()) / 2)
                     : feature::kInvalidAltitude;
      res.m_projPoint = Junction(pt, projPointAlt);
    }
  }

  if (res.m_fid.IsValid())
    m_candidates.push_back(res);
}

void NearestEdgeFinder::MakeResult(vector<pair<Edge, Junction>> & res,
                                   size_t const maxCountFeatures)
{
  sort(m_candidates.begin(), m_candidates.end(), [](Candidate const & r1, Candidate const & r2)
  {
    return (r1.m_dist < r2.m_dist);
  });

  res.clear();
  res.reserve(maxCountFeatures);

  size_t i = 0;
  for (Candidate const & candidate : m_candidates)
  {
    res.emplace_back(Edge(candidate.m_fid, true /* forward */, candidate.m_segId,
                          candidate.m_segStart, candidate.m_segEnd),
                     candidate.m_projPoint);
    ++i;
    if (i == maxCountFeatures)
      break;
  }
}

}  // namespace routing
