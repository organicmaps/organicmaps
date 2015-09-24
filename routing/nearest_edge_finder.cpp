#include "routing/nearest_edge_finder.hpp"

#include "geometry/distance.hpp"

#include "indexer/feature.hpp"

#include "base/assert.hpp"

#include "std/limits.hpp"

namespace routing
{

NearestEdgeFinder::Candidate::Candidate()
    : m_dist(numeric_limits<double>::max()),
      m_segId(0),
      m_segStart(m2::PointD::Zero()),
      m_segEnd(m2::PointD::Zero()),
      m_point(m2::PointD::Zero())
{
}

NearestEdgeFinder::NearestEdgeFinder(m2::PointD const & point)
    : m_point(point)
{
}

void NearestEdgeFinder::AddInformationSource(FeatureID const & featureId, IRoadGraph::RoadInfo const & roadInfo)
{
  Candidate res;

  size_t const count = roadInfo.m_points.size();
  ASSERT_GREATER(count, 1, ());
  for (size_t i = 1; i < count; ++i)
  {
    /// @todo Probably, we need to get exact projection distance in meters.
    m2::ProjectionToSection<m2::PointD> segProj;
    segProj.SetBounds(roadInfo.m_points[i - 1], roadInfo.m_points[i]);

    m2::PointD const pt = segProj(m_point);
    double const d = m_point.SquareLength(pt);
    if (d < res.m_dist)
    {
      res.m_dist = d;
      res.m_fid = featureId;
      res.m_segId = static_cast<uint32_t>(i - 1);
      res.m_segStart = roadInfo.m_points[i - 1];
      res.m_segEnd = roadInfo.m_points[i];
      res.m_point = pt;
    }
  }

  if (res.m_fid.IsValid())
    m_candidates.push_back(res);
}

void NearestEdgeFinder::MakeResult(vector<pair<Edge, m2::PointD>> & res, size_t const maxCountFeatures)
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
    res.emplace_back(Edge(candidate.m_fid, true /* forward */, candidate.m_segId, candidate.m_segStart, candidate.m_segEnd), candidate.m_point);
    ++i;
    if (i == maxCountFeatures)
      break;
  }
}

}  // namespace routing
