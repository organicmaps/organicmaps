#include "routing/nearest_edge_finder.hpp"

#include "geometry/parametrized_segment.hpp"

#include "indexer/feature.hpp"

#include "base/assert.hpp"

namespace routing
{
using namespace std;

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
    m2::ParametrizedSegment<m2::PointD> segment(roadInfo.m_junctions[i - 1].GetPoint(),
                                                roadInfo.m_junctions[i].GetPoint());

    m2::PointD const pt = segment.ClosestPointTo(m_point);
    double const d = m_point.SquaredLength(pt);
    if (d < res.m_dist)
    {
      Junction const & segStart = roadInfo.m_junctions[i - 1];
      Junction const & segEnd = roadInfo.m_junctions[i];
      feature::TAltitude const startAlt = segStart.GetAltitude();
      feature::TAltitude const endAlt = segEnd.GetAltitude();

      double const segLenM = MercatorBounds::DistanceOnEarth(segStart.GetPoint(), segEnd.GetPoint());
      feature::TAltitude projPointAlt = feature::kDefaultAltitudeMeters;
      if (segLenM == 0.0)
      {
        projPointAlt = startAlt;
      }
      else
      {
        double const distFromStartM = MercatorBounds::DistanceOnEarth(segStart.GetPoint(), pt);
        ASSERT_LESS_OR_EQUAL(distFromStartM, segLenM, (featureId));
        projPointAlt = startAlt + static_cast<feature::TAltitude>((endAlt - startAlt) * distFromStartM / segLenM);
      }

      res.m_dist = d;
      res.m_fid = featureId;
      res.m_segId = static_cast<uint32_t>(i - 1);
      res.m_segStart = segStart;
      res.m_segEnd = segEnd;
      res.m_bidirectional = roadInfo.m_bidirectional;

      ASSERT_NOT_EQUAL(res.m_segStart.GetAltitude() , feature::kInvalidAltitude, ());
      ASSERT_NOT_EQUAL(res.m_segEnd.GetAltitude(), feature::kInvalidAltitude, ());

      res.m_projPoint = Junction(pt, projPointAlt);
    }
  }

  if (res.m_fid.IsValid())
    m_candidates.push_back(res);
}

void NearestEdgeFinder::MakeResult(vector<pair<Edge, Junction>> & res, size_t const maxCountFeatures)
{
  sort(m_candidates.begin(), m_candidates.end(), [](Candidate const & r1, Candidate const & r2)
  {
    return (r1.m_dist < r2.m_dist);
  });

  res.clear();
  res.reserve(maxCountFeatures);
  
  for (Candidate const & candidate : m_candidates)
  {
    res.emplace_back(Edge::MakeReal(candidate.m_fid, true /* forward */, candidate.m_segId,
                                    candidate.m_segStart, candidate.m_segEnd),
                     candidate.m_projPoint);
    if (res.size() >= maxCountFeatures)
      return;

    if (candidate.m_bidirectional)
    {
      res.emplace_back(Edge::MakeReal(candidate.m_fid, false /* forward */, candidate.m_segId,
                                      candidate.m_segEnd, candidate.m_segStart),
                       candidate.m_projPoint);
      if (res.size() >= maxCountFeatures)
        return;
    }
  }
}
}  // namespace routing
