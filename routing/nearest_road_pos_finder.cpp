#include "routing/nearest_road_pos_finder.hpp"

#include "geometry/distance.hpp"

#include "indexer/feature.hpp"

#include "base/assert.hpp"

namespace routing
{
void NearestRoadPosFinder::AddInformationSource(const FeatureType & ft)
{
  if (ft.GetFeatureType() != feature::GEOM_LINE || m_vehicleModel->GetSpeed(ft) == 0.0)
    return;

  Candidate res;

  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

  size_t const count = ft.GetPointsCount();
  ASSERT_GREATER(count, 1, ());
  for (size_t i = 1; i < count; ++i)
  {
    /// @todo Probably, we need to get exact projection distance in meters.
    m2::ProjectionToSection<m2::PointD> segProj;
    segProj.SetBounds(ft.GetPoint(i - 1), ft.GetPoint(i));

    m2::PointD const pt = segProj(m_point);
    double const d = m_point.SquareLength(pt);
    if (d < res.m_dist)
    {
      res.m_dist = d;
      res.m_fid = ft.GetID().m_offset;
      res.m_segId = i - 1;
      res.m_point = pt;
      res.m_isOneway = m_vehicleModel->IsOneWay(ft);
      if (!m_mwmId.IsAlive())
        m_mwmId = ft.GetID().m_mwmId;
      ASSERT_EQUAL(ft.GetID().m_mwmId, m_mwmId, ());
    }
  }

  if (res.m_fid != INVALID_FID)
    m_candidates.push_back(res);
}

void NearestRoadPosFinder::MakeResult(vector<RoadPos> & res, const size_t maxCount)
{
  if (!m_mwmId.IsAlive())
    return;
  sort(m_candidates.begin(), m_candidates.end(), [](Candidate const & r1, Candidate const & r2)
  {
    return (r1.m_dist < r2.m_dist);
  });

  res.clear();
  res.reserve(maxCount);

  for (Candidate const & candidate : m_candidates)
  {
    if (res.size() == maxCount)
      break;
    res.push_back(RoadPos(candidate.m_fid, true, candidate.m_segId, candidate.m_point));
    if (res.size() == maxCount)
      break;
    if (candidate.m_isOneway)
      continue;
    res.push_back(RoadPos(candidate.m_fid, false, candidate.m_segId, candidate.m_point));
  }
}

}  // namespace routing
