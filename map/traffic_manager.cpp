#include "map/traffic_manager.hpp"

#include "routing/routing_helpers.hpp"

#include "drape_frontend/drape_engine.hpp"

#include "indexer/scales.hpp"

namespace
{
seconds const kUpdateInterval = minutes(5);
}  // namespace

TrafficManager::TrafficManager(Index const & index,
                               GetMwmsByRectFn const & getMwmsByRectFn)
  : m_isEnabled(true) //TODO: true is temporary
  , m_index(index)
  , m_getMwmsByRectFn(getMwmsByRectFn)
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
  m_requestTimings.clear();
  m_mwmIds.clear();

  UpdateViewport(m_currentModelView);
  UpdateMyPosition(m_currentPosition);
}

void TrafficManager::UpdateViewport(ScreenBase const & screen)
{
  m_currentModelView = screen;

  if (!m_isEnabled)
    return;

  // Request traffic.
  auto mwms = m_getMwmsByRectFn(screen.ClipRect());
  for (auto const & mwm : mwms)
  {
    if (mwm.IsAlive())
      RequestTrafficData(mwm);
  }

  // TODO: Remove some mwm's from cache.
  //MwmSet::MwmId mwmId;
  //m_drapeEngine->ClearTrafficCache(mwmId);
  // TODO: remove from m_requestTimings
  // TODO: remove from m_mwmIds
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
      if (polyline.GetSize() > 1)
        polylines[fid] = polyline;
    }
  };
  m_index.ReadFeatures(featureReader, features);

  for (auto const & c : trafficInfo.GetColoring())
  {
    auto it = polylines.find(c.first.m_fid);
    if (it == polylines.end())
      continue;
    bool const isReversed = (c.first.m_dir == traffic::TrafficInfo::RoadSegmentId::kReverseDirection);
    m2::PolylineD polyline = it->second.ExtractSegment(c.first.m_idx, isReversed);
    if (polyline.GetSize() > 1)
      output.emplace_back(df::TrafficSegmentID(trafficInfo.GetMwmId(), c.first), move(polyline));
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
    mwms.reserve(m_requestTimings.size());
    for (auto const & timing : m_requestTimings)
      mwms.push_back(timing.first);
  }
  else
  {
    ASSERT(!m_requestedMwms.empty(), ());
    mwms.swap(m_requestedMwms);
  }
  return true;
}

void TrafficManager::RequestTrafficData(MwmSet::MwmId const & mwmId)
{
  lock_guard<mutex> lock(m_requestedMwmsLock);

  ASSERT(mwmId.IsAlive(), ());
  bool needRequesting = false;

  auto it = m_requestTimings.find(mwmId);
  if (it == m_requestTimings.end())
  {
    needRequesting = true;
    m_requestTimings[mwmId] = steady_clock::now();
  }
  else
  {
    auto const passedSeconds = steady_clock::now() - it->second;
    if (passedSeconds >= kUpdateInterval)
    {
      needRequesting = true;
      it->second = steady_clock::now();
    }
  }

  if (needRequesting)
    RequestTrafficDataImpl(mwmId);
}

void TrafficManager::RequestTrafficDataImpl(MwmSet::MwmId const & mwmId)
{
  m_requestedMwms.push_back(mwmId);
  m_condition.notify_one();
}

void TrafficManager::OnTrafficDataResponse(traffic::TrafficInfo const & info)
{
  // Cache geometry for rendering if it's necessary.
  if (m_mwmIds.find(info.GetMwmId()) == m_mwmIds.end())
  {
    df::TrafficSegmentsGeometry geometry;
    CalculateSegmentsGeometry(info, geometry);
    m_mwmIds.insert(info.GetMwmId());
    m_drapeEngine->CacheTrafficSegmentsGeometry(geometry);
  }

  // Update traffic colors.
  df::TrafficSegmentsColoring coloring;
  CalculateSegmentsColoring(info, coloring);
  m_drapeEngine->UpdateTraffic(coloring);
}
