#include "map/traffic_manager.hpp"

#include "routing/routing_helpers.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

namespace
{
auto const kUpdateInterval = minutes(1);
}  // namespace

TrafficManager::TrafficManager(GetMwmsByRectFn const & getMwmsByRectFn, size_t maxCacheSizeBytes)
  : m_isEnabled(false)
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
  if (m_drapeEngine != nullptr)
    m_drapeEngine->EnableTraffic(enabled);
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

  if (df::GetZoomLevel(screen.GetScale()) < df::kRoadClass0ZoomLevel)
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

  // Update cache.
  size_t constexpr kElementSize = sizeof(traffic::TrafficInfo::RoadSegmentId) + sizeof(traffic::SpeedGroup);
  size_t const dataSize = info.GetColoring().size() * kElementSize;
  it->second.m_isLoaded = true;
  m_currentCacheSizeBytes += (dataSize - it->second.m_dataSize);
  it->second.m_dataSize = dataSize;
  CheckCacheSize();

  // Update traffic colors.
  df::TrafficSegmentsColoring coloring;
  coloring[info.GetMwmId()] = info.GetColoring();
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
