#include "map/traffic_manager.hpp"

#include "routing/routing_helpers.hpp"

#include "drape_frontend/drape_engine.hpp"

#include "indexer/scales.hpp"

void TrafficManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine = engine;
}

void TrafficManager::UpdateViewport(ScreenBase const & screen)
{
  // 1. Determine mwm's inside viewport.

  // 2. Request traffic for this mwm's.

  // 3. Cache geometry for rendering if it's necessary.
  //MwmSet::MwmId mwmId;
  //if (m_mwmIds.find(mwmId) == m_mwmIds.end())
  //{
  //  df::TrafficSegmentsGeometry geometry;
  //  traffic::TrafficInfo info;
  //  CalculateSegmentsGeometry(info, geometry);
  //  m_mwmIds.insert(mwmId);
  //  m_drapeEngine->CacheTrafficSegmentsGeometry(geometry);
  //}

  // 4. Update traffic colors.
  //df::TrafficSegmentsColoring coloring;
  //traffic::TrafficInfo info;
  //CalculateSegmentsColoring(info, coloring);
  //m_drapeEngine->UpdateTraffic(coloring);

  // 5. Remove some mwm's from cache.
  //MwmSet::MwmId mwmId;
  //m_drapeEngine->ClearTrafficCache(mwmId);
}

void TrafficManager::UpdateMyPosition(MyPosition const & myPosition)
{
  // 1. Determine mwm's nearby "my position".

  // 2. Request traffic for this mwm's.

  // 3. Do all routing stuff.
}

void TrafficManager::CalculateSegmentsGeometry(traffic::TrafficInfo const & trafficInfo,
                                               df::TrafficSegmentsGeometry & output) const
{
  size_t const coloringSize = trafficInfo.GetColoring().size();
  output.reserve(coloringSize);

  vector<FeatureID> features;
  features.reserve(coloringSize);
  for (auto const & c : trafficInfo.GetColoring())
    features.emplace_back(trafficInfo.GetMwmId(), c.first.m_fid);

  int constexpr kScale = scales::GetUpperScale();
  unordered_map<uint32_t, m2::PolylineD> polylines;
  auto featureReader = [&polylines](FeatureType & ft)
  {
    uint32_t const fid = ft.GetID().m_index;
    if (polylines.find(fid) != polylines.end())
      return;

    if (routing::IsRoad(feature::TypesHolder(ft)))
    {
      m2::PolylineD polyline;
      ft.ForEachPoint([&polyline](m2::PointD const & pt) { polyline.Add(pt); }, kScale);
      polylines[fid] = polyline;
    }
  };
  m_index.ReadFeatures(featureReader, features);

  for (auto const & c : trafficInfo.GetColoring())
  {
    output.push_back(make_pair(df::TrafficSegmentID(trafficInfo.GetMwmId(), c.first),
                               polylines[c.first.m_fid].ExtractSegment(c.first.m_idx,
                                                                       c.first.m_dir == 1)));
  }
}

void TrafficManager::CalculateSegmentsColoring(traffic::TrafficInfo const & trafficInfo,
                                               df::TrafficSegmentsColoring & output) const
{
  for (auto const & c : trafficInfo.GetColoring())
  {
    df::TrafficSegmentID const sid (trafficInfo.GetMwmId(), c.first);
    output.emplace_back(sid, c.second);
  }
}
