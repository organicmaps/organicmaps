#include "map/traffic_manager.hpp"

#include "routing/routing_helpers.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

namespace
{
auto const kUpdateInterval = minutes(1);

int const kMinTrafficZoom = 10;

}  // namespace

TrafficManager::TrafficManager(Index const & index, GetMwmsByRectFn const & getMwmsByRectFn,
                               size_t maxCacheSizeBytes)
  : m_isEnabled(true)  // TODO: true is temporary
  , m_index(index)
  , m_getMwmsByRectFn(getMwmsByRectFn)
  , m_maxCacheSizeBytes(maxCacheSizeBytes)
  , m_currentCacheSizeBytes(0)
  , m_isRunning(true)
  , m_thread(&TrafficManager::ThreadRoutine, this)
{
  CHECK(m_getMwmsByRectFn != nullptr, ());
}

TrafficManager::~TrafficManager()
{
  {
    lock_guard<mutex> lock(m_requestedMwmsLock);
    m_isRunning = false;
  }
  m_condition.notify_one();
  m_thread.join();
}

void TrafficManager::SetEnabled(bool enabled)
{
  m_isEnabled = enabled;
}

void TrafficManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine = engine;
}

void TrafficManager::OnRecover()
{
  m_mwmInfos.clear();

  UpdateViewport(m_currentModelView);
  UpdateMyPosition(m_currentPosition);
}

void TrafficManager::UpdateViewport(ScreenBase const & screen)
{
  m_currentModelView = screen;

  if (!m_isEnabled)
    return;

  if (df::GetZoomLevel(screen.GetScale()) < kMinTrafficZoom)
    return;

  // Request traffic.
  auto mwms = m_getMwmsByRectFn(screen.ClipRect());

  {
    lock_guard<mutex> lock(m_requestedMwmsLock);

    m_activeMwms.clear();
    for (auto const & mwm : mwms)
    {
      if (mwm.IsAlive())
        m_activeMwms.push_back(mwm);
    }

    RequestTrafficData();
  }
}

void TrafficManager::UpdateMyPosition(MyPosition const & myPosition)
{
  m_currentPosition = myPosition;

  if (!m_isEnabled)
    return;

  // 1. Determine mwm's nearby "my position".

  // 2. Request traffic for this mwm's.

  // 3. Do all routing stuff.
}

void TrafficManager::CalculateSegmentsGeometry(traffic::TrafficInfo const & trafficInfo,
                                               df::TrafficSegmentsGeometry & output) const
{
  size_t const coloringSize = trafficInfo.GetColoring().size();

  vector<FeatureID> features;
  features.reserve(coloringSize);
  for (auto const & c : trafficInfo.GetColoring())
    features.emplace_back(trafficInfo.GetMwmId(), c.first.m_fid);

  int constexpr kScale = scales::GetUpperScale();
  unordered_map<uint32_t, pair<m2::PolylineD, df::RoadClass>> polylines;
  auto featureReader = [&polylines](FeatureType & ft)
  {
    uint32_t const fid = ft.GetID().m_index;
    if (polylines.find(fid) != polylines.end())
      return;

    if (routing::IsRoad(feature::TypesHolder(ft)))
    {
      auto const highwayClass = ftypes::GetHighwayClass(ft);
      df::RoadClass roadClass = df::RoadClass::Class2;
      if (highwayClass == ftypes::HighwayClass::Trunk || highwayClass == ftypes::HighwayClass::Primary)
        roadClass = df::RoadClass::Class0;
      else if (highwayClass == ftypes::HighwayClass::Secondary || highwayClass == ftypes::HighwayClass::Tertiary)
        roadClass = df::RoadClass::Class1;

      m2::PolylineD polyline;
      ft.ForEachPoint([&polyline](m2::PointD const & pt) { polyline.Add(pt); }, kScale);
      if (polyline.GetSize() > 1)
        polylines[fid] = make_pair(polyline, roadClass);
    }
  };
  m_index.ReadFeatures(featureReader, features);

  for (auto const & c : trafficInfo.GetColoring())
  {
    auto it = polylines.find(c.first.m_fid);
    if (it == polylines.end())
      continue;
    bool const isReversed = (c.first.m_dir == traffic::TrafficInfo::RoadSegmentId::kReverseDirection);
    m2::PolylineD polyline = it->second.first.ExtractSegment(c.first.m_idx, isReversed);
    if (polyline.GetSize() > 1)
      output.insert(make_pair(df::TrafficSegmentID(trafficInfo.GetMwmId(), c.first),
                              df::TrafficSegmentGeometry(move(polyline), it->second.second)));
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

void TrafficManager::ThreadRoutine()
{
  vector<MwmSet::MwmId> mwms;
  while (WaitForRequest(mwms))
  {
    for (auto const & mwm : mwms)
    {
      traffic::TrafficInfo info(mwm);

      if (info.ReceiveTrafficData())
        OnTrafficDataResponse(info);
      else
        LOG(LWARNING, ("Traffic request failed. Mwm =", mwm));
    }
    mwms.clear();
  }
}

bool TrafficManager::WaitForRequest(vector<MwmSet::MwmId> & mwms)
{
  unique_lock<mutex> lock(m_requestedMwmsLock);
  bool const timeout = !m_condition.wait_for(lock, kUpdateInterval, [this] { return !m_isRunning || !m_requestedMwms.empty(); });
  if (!m_isRunning)
    return false;
  if (timeout)
  {
    mwms = m_activeMwms;
  }
  else
  {
    ASSERT(!m_requestedMwms.empty(), ());
    mwms.swap(m_requestedMwms);
  }
  return true;
}

void TrafficManager::RequestTrafficData()
{
  if (m_activeMwms.empty())
    return;

  for (auto const & mwmId : m_activeMwms)
  {
    ASSERT(mwmId.IsAlive(), ());
    bool needRequesting = false;

    auto it = m_mwmInfos.find(mwmId);
    if (it == m_mwmInfos.end())
    {
      needRequesting = true;
      m_mwmInfos.insert(make_pair(mwmId, MwmTrafficInfo(steady_clock::now())));
    }
    else
    {
      auto const passedSeconds = steady_clock::now() - it->second.m_lastRequestTime;
      if (passedSeconds >= kUpdateInterval)
      {
        needRequesting = true;
        it->second.m_lastRequestTime = steady_clock::now();
      }
    }

    if (needRequesting)
      RequestTrafficData(mwmId);
  }
}

void TrafficManager::RequestTrafficData(MwmSet::MwmId const & mwmId)
{
  m_requestedMwms.push_back(mwmId);
  m_condition.notify_one();
}

void TrafficManager::OnTrafficDataResponse(traffic::TrafficInfo const & info)
{
  auto it = m_mwmInfos.find(info.GetMwmId());
  if (it == m_mwmInfos.end())
    return;

  // Cache geometry for rendering if it's necessary.
  if (!it->second.m_isLoaded)
  {
    df::TrafficSegmentsGeometry geometry;
    CalculateSegmentsGeometry(info, geometry);
    it->second.m_isLoaded = true;
    m_drapeEngine->CacheTrafficSegmentsGeometry(geometry);
  }

  // Update traffic colors.
  df::TrafficSegmentsColoring coloring;
  CalculateSegmentsColoring(info, coloring);

  size_t dataSize = coloring.size() * sizeof(df::TrafficSegmentColoring);
  it->second.m_dataSize = dataSize;
  m_currentCacheSizeBytes += dataSize;

  CheckCacheSize();

  m_drapeEngine->UpdateTraffic(coloring);
}

void TrafficManager::CheckCacheSize()
{
  if ((m_currentCacheSizeBytes > m_maxCacheSizeBytes) && (m_mwmInfos.size() > m_activeMwms.size()))
  {
    std::multimap<time_point<steady_clock>, MwmSet::MwmId> seenTimings;
    for (auto const & mwmInfo : m_mwmInfos)
      seenTimings.insert(make_pair(mwmInfo.second.m_lastSeenTime, mwmInfo.first));

    auto itSeen = seenTimings.begin();
    while ((m_currentCacheSizeBytes > m_maxCacheSizeBytes) &&
           (m_mwmInfos.size() > m_activeMwms.size()))
    {
      auto const mwmId = itSeen->second;
      auto const it = m_mwmInfos.find(mwmId);
      if (it->second.m_isLoaded)
      {
        m_currentCacheSizeBytes -= it->second.m_dataSize;
        m_drapeEngine->ClearTrafficCache(mwmId);
      }
      m_mwmInfos.erase(it);
      ++itSeen;
    }
  }
}
