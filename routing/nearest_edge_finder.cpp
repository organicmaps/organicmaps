#include "routing/nearest_edge_finder.hpp"

#include "geometry/parametrized_segment.hpp"

#include "indexer/feature.hpp"

#include "base/assert.hpp"

namespace routing
{
using namespace std;

NearestEdgeFinder::NearestEdgeFinder(m2::PointD const & point, IsEdgeProjGood const & isEdgeProjGood)
    : m_point(point), m_isEdgeProjGood(isEdgeProjGood)
{
}

void NearestEdgeFinder::AddInformationSource(IRoadGraph::FullRoadInfo const & roadInfo)
{
  if (!roadInfo.m_featureId.IsValid())
    return;

  Candidate res;

  auto const & junctions = roadInfo.m_roadInfo.m_junctions;
  size_t const count = junctions.size();
  ASSERT_GREATER(count, 1, ());
  for (size_t i = 1; i < count; ++i)
  {
    m2::ParametrizedSegment<m2::PointD> segment(junctions[i - 1].GetPoint(),
                                                junctions[i].GetPoint());

    m2::PointD const closestPoint = segment.ClosestPointTo(m_point);
    double const squaredDist = m_point.SquaredLength(closestPoint);

    if (squaredDist < res.m_squaredDist)
    {
      res.m_segId = static_cast<uint32_t>(i - 1);
      res.m_squaredDist = squaredDist;
    }
  }

  if (res.m_segId == Candidate::kInvalidSegmentId)
    return;

  // Closest point to |this->m_point| found. It has index |res.m_segId + 1| in |junctions|.
  size_t const idx = res.m_segId + 1;
  Junction const & segStart = junctions[idx - 1];
  Junction const & segEnd = junctions[idx];
  feature::TAltitude const startAlt = segStart.GetAltitude();
  feature::TAltitude const endAlt = segEnd.GetAltitude();
  m2::ParametrizedSegment<m2::PointD> segment(junctions[idx - 1].GetPoint(),
                                              junctions[idx].GetPoint());
  m2::PointD const closestPoint = segment.ClosestPointTo(m_point);

  double const segLenM = mercator::DistanceOnEarth(segStart.GetPoint(), segEnd.GetPoint());
  feature::TAltitude projPointAlt = feature::kDefaultAltitudeMeters;
  if (segLenM == 0.0)
  {
    projPointAlt = startAlt;
  }
  else
  {
    double const distFromStartM = mercator::DistanceOnEarth(segStart.GetPoint(), closestPoint);
    ASSERT_LESS_OR_EQUAL(distFromStartM, segLenM, (roadInfo.m_featureId));
    projPointAlt =
        startAlt + static_cast<feature::TAltitude>((endAlt - startAlt) * distFromStartM / segLenM);
  }

  res.m_fid = roadInfo.m_featureId;
  res.m_segStart = segStart;
  res.m_segEnd = segEnd;
  res.m_bidirectional = roadInfo.m_roadInfo.m_bidirectional;

  ASSERT_NOT_EQUAL(res.m_segStart.GetAltitude(), feature::kInvalidAltitude, ());
  ASSERT_NOT_EQUAL(res.m_segEnd.GetAltitude(), feature::kInvalidAltitude, ());
  res.m_projPoint = Junction(closestPoint, projPointAlt);

  m_candidates.emplace_back(res);
}

void NearestEdgeFinder::MakeResult(vector<pair<Edge, Junction>> & res, size_t maxCountFeatures)
{
  sort(m_candidates.begin(), m_candidates.end(), [](Candidate const & r1, Candidate const & r2)
  {
    return r1.m_squaredDist < r2.m_squaredDist;
  });

  res.clear();
  res.reserve(maxCountFeatures);
  
  for (Candidate const & candidate : m_candidates)
  {
    CandidateToResult(candidate, maxCountFeatures, res);
    if (res.size() >= maxCountFeatures)
      return;
  }
}

void NearestEdgeFinder::CandidateToResult(Candidate const & candidate,
                                          size_t maxCountFeatures,
                                          vector<pair<Edge, Junction>> & res) const
{
  AddResIf(candidate, true /* forward */, maxCountFeatures, res);

  if (candidate.m_bidirectional)
    AddResIf(candidate, false /* forward */, maxCountFeatures, res);
}

void NearestEdgeFinder::AddResIf(Candidate const & candidate, bool forward, size_t maxCountFeatures,
                                 vector<pair<Edge, Junction>> & res) const
{
  if (res.size() >= maxCountFeatures)
    return;

  Junction const & start = forward ? candidate.m_segStart : candidate.m_segEnd;
  Junction const & end = forward ? candidate.m_segEnd : candidate.m_segStart;
  auto const & edge = Edge::MakeReal(candidate.m_fid, forward, candidate.m_segId, start,end);
  auto const & edgeProj = make_pair(edge, candidate.m_projPoint);
  if (m_isEdgeProjGood && !m_isEdgeProjGood(edgeProj))
    return;

  res.emplace_back(edgeProj);
}
}  // namespace routing
