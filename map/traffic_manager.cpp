#include "map/traffic_manager.hpp"

#include "routing/routing_helpers.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "platform/platform.hpp"

namespace
{
auto constexpr kUpdateInterval = minutes(1);
auto constexpr kOutdatedDataTimeout = minutes(5) + kUpdateInterval;
auto constexpr kNetworkErrorTimeout = minutes(20);

auto constexpr kMaxRetriesCount = 5;
} // namespace


TrafficManager::CacheEntry::CacheEntry()
  : m_isLoaded(false)
  , m_dataSize(0)
  , m_retriesCount(0)
  , m_isWaitingForResponse(false)
  , m_lastAvailability(traffic::TrafficInfo::Availability::Unknown)
{}

TrafficManager::CacheEntry::CacheEntry(time_point<steady_clock> const & requestTime)
  : m_isLoaded(false)
  , m_dataSize(0)
  , m_lastRequestTime(requestTime)
  , m_retriesCount(0)
  , m_isWaitingForResponse(true)
  , m_lastAvailability(traffic::TrafficInfo::Availability::Unknown)
{}

TrafficManager::TrafficManager(GetMwmsByRectFn const & getMwmsByRectFn, size_t maxCacheSizeBytes,
                               traffic::TrafficObserver & observer)
  : m_getMwmsByRectFn(getMwmsByRectFn)
  , m_observer(observer)
  , m_currentDataVersion(0)
  , m_state(TrafficState::Disabled)
  , m_maxCacheSizeBytes(maxCacheSizeBytes)
  , m_isRunning(true)
  , m_thread(&TrafficManager::ThreadRoutine, this)
{
  CHECK(m_getMwmsByRectFn != nullptr, ());
}

TrafficManager::~TrafficManager()
{
#ifdef DEBUG
  {
    lock_guard<mutex> lock(m_mutex);
    ASSERT(!m_isRunning, ());
  }
#endif
}

void TrafficManager::Teardown()
{
  {
    lock_guard<mutex> lock(m_mutex);
    if (!m_isRunning)
      return;
    m_isRunning = false;
  }
  m_condition.notify_one();
  m_thread.join();
}

void TrafficManager::SetStateListener(TrafficStateChangedFn const & onStateChangedFn)
{
  GetPlatform().RunOnGuiThread([this, onStateChangedFn]()
  {
    m_onStateChangedFn = onStateChangedFn;
  });
}

void TrafficManager::SetEnabled(bool enabled)
{
  {
    lock_guard<mutex> lock(m_mutex);
    if (enabled == IsEnabled())
    {
       LOG(LWARNING, ("Invalid attempt to", enabled ? "enable" : "disable",
                      "traffic manager, it's already", enabled ? "enabled" : "disabled",
                      ", doing nothing."));
       return;
    }

    Clear();
    ChangeState(enabled ? TrafficState::Enabled : TrafficState::Disabled);
  }

  if (m_drapeEngine != nullptr)
    m_drapeEngine->EnableTraffic(enabled);

  if (enabled)
  {
    if (m_currentModelView.second)
      UpdateViewport(m_currentModelView.first);
    if (m_currentPosition.second)
      UpdateMyPosition(m_currentPosition.first);
  }

  m_observer.OnTrafficEnabled(enabled);
}

void TrafficManager::Clear()
{
  m_mwmCache.clear();
  m_lastMwmsByRect.clear();
  m_activeMwms.clear();
  m_requestedMwms.clear();
}

void TrafficManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine = engine;
}

void TrafficManager::SetCurrentDataVersion(int64_t dataVersion)
{
  m_currentDataVersion = dataVersion;
}

void TrafficManager::OnDestroyGLContext()
{
  if (!IsEnabled())
    return;

  lock_guard<mutex> lock(m_mutex);
  Clear();
}

void TrafficManager::OnRecoverGLContext()
{
  if (!IsEnabled())
    return;

  if (m_currentModelView.second)
    UpdateViewport(m_currentModelView.first);
  if (m_currentPosition.second)
    UpdateMyPosition(m_currentPosition.first);
}

void TrafficManager::UpdateViewport(ScreenBase const & screen)
{
  m_currentModelView = {screen, true /* initialized */};

  if (!IsEnabled() || IsInvalidState())
    return;

  if (df::GetZoomLevel(screen.GetScale()) < df::kRoadClass0ZoomLevel)
    return;

  // Request traffic.
  auto mwms = m_getMwmsByRectFn(screen.ClipRect());
  if (m_lastMwmsByRect == mwms)
    return;
  m_lastMwmsByRect = mwms;

  {
    lock_guard<mutex> lock(m_mutex);

    m_activeMwms.clear();
    for (auto const & mwm : mwms)
    {
      if (mwm.IsAlive())
        m_activeMwms.insert(mwm);
    }

    RequestTrafficData();
  }
}

void TrafficManager::UpdateMyPosition(MyPosition const & myPosition)
{
  m_currentPosition = {myPosition, true /* initialized */};

  if (!IsEnabled() || IsInvalidState())
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
      traffic::TrafficInfo info(mwm, m_currentDataVersion);

      if (info.ReceiveTrafficData())
      {
        OnTrafficDataResponse(move(info));
      }
      else
      {
        LOG(LWARNING, ("Traffic request failed. Mwm =", mwm));
        OnTrafficRequestFailed(move(info));
      }
    }
    mwms.clear();
  }
}

bool TrafficManager::WaitForRequest(vector<MwmSet::MwmId> & mwms)
{
  unique_lock<mutex> lock(m_mutex);

  bool const timeout = !m_condition.wait_for(lock, kUpdateInterval, [this]
  {
    return !m_isRunning || !m_requestedMwms.empty();
  });

  if (!m_isRunning)
    return false;

  if (timeout)
    RequestTrafficData();

  if (!m_requestedMwms.empty())
    mwms.swap(m_requestedMwms);

  return true;
}

void TrafficManager::RequestTrafficData(MwmSet::MwmId const & mwmId)
{
  bool needRequesting = false;
  auto const currentTime = steady_clock::now();
  auto const it = m_mwmCache.find(mwmId);
  if (it == m_mwmCache.end())
  {
    needRequesting = true;
    m_mwmCache.insert(make_pair(mwmId, CacheEntry(currentTime)));
  }
  else
  {
    it->second.m_lastSeenTime = currentTime;
    auto const passedSeconds = currentTime - it->second.m_lastRequestTime;
    if (passedSeconds >= kUpdateInterval)
    {
      needRequesting = true;
      it->second.m_isWaitingForResponse = true;
      it->second.m_lastRequestTime = currentTime;
    }
  }

  if (needRequesting)
  {
    m_requestedMwms.push_back(mwmId);
    m_condition.notify_one();
  }
}

void TrafficManager::RequestTrafficData()
{
  if (m_activeMwms.empty() || !IsEnabled() || IsInvalidState())
    return;

  for (auto const & mwmId : m_activeMwms)
  {
    ASSERT(mwmId.IsAlive(), ());
    RequestTrafficData(mwmId);
  }
  UpdateState();
}

void TrafficManager::OnTrafficRequestFailed(traffic::TrafficInfo && info)
{
  lock_guard<mutex> lock(m_mutex);

  auto it = m_mwmCache.find(info.GetMwmId());
  if (it == m_mwmCache.end())
    return;

  it->second.m_isWaitingForResponse = false;
  it->second.m_lastAvailability = info.GetAvailability();

  if (info.GetAvailability() == traffic::TrafficInfo::Availability::Unknown &&
      !it->second.m_isLoaded)
  {
    if (m_activeMwms.find(info.GetMwmId()) != m_activeMwms.end())
    {
      if (it->second.m_retriesCount < kMaxRetriesCount)
      {
        it->second.m_lastRequestTime = steady_clock::now();
        it->second.m_isWaitingForResponse = true;
        RequestTrafficData(info.GetMwmId());
      }
      ++it->second.m_retriesCount;
    }
    else
    {
      it->second.m_retriesCount = 0;
    }
  }

  UpdateState();
}

void TrafficManager::OnTrafficDataResponse(traffic::TrafficInfo && info)
{
  {
    lock_guard<mutex> lock(m_mutex);

    auto it = m_mwmCache.find(info.GetMwmId());
    if (it == m_mwmCache.end())
      return;

    it->second.m_isLoaded = true;
    it->second.m_lastResponseTime = steady_clock::now();
    it->second.m_isWaitingForResponse = false;
    it->second.m_lastAvailability = info.GetAvailability();

    if (!info.GetColoring().empty())
    {
      // Update cache.
      size_t constexpr kElementSize = sizeof(traffic::TrafficInfo::RoadSegmentId) + sizeof(traffic::SpeedGroup);
      size_t const dataSize = info.GetColoring().size() * kElementSize;
      m_currentCacheSizeBytes += (dataSize - it->second.m_dataSize);
      it->second.m_dataSize = dataSize;
      CheckCacheSize();
    }

    UpdateState();
  }

  if (!info.GetColoring().empty())
  {
    m_drapeEngine->UpdateTraffic(info);

    // Update traffic colors for routing.
    m_observer.OnTrafficInfoAdded(move(info));
  }
}

void TrafficManager::CheckCacheSize()
{
  if (m_currentCacheSizeBytes > m_maxCacheSizeBytes && m_mwmCache.size() > m_activeMwms.size())
  {
    std::multimap<time_point<steady_clock>, MwmSet::MwmId> seenTimings;
    for (auto const & mwmInfo : m_mwmCache)
      seenTimings.insert(make_pair(mwmInfo.second.m_lastSeenTime, mwmInfo.first));

    auto itSeen = seenTimings.begin();
    while (m_currentCacheSizeBytes > m_maxCacheSizeBytes &&
           m_mwmCache.size() > m_activeMwms.size())
    {
      auto const mwmId = itSeen->second;
      auto const it = m_mwmCache.find(mwmId);
      if (it->second.m_isLoaded)
      {
        ASSERT_GREATER_OR_EQUAL(m_currentCacheSizeBytes, it->second.m_dataSize, ());
        m_currentCacheSizeBytes -= it->second.m_dataSize;
        m_drapeEngine->ClearTrafficCache(mwmId);
        m_observer.OnTrafficInfoRemoved(mwmId);
      }
      m_mwmCache.erase(it);
      ++itSeen;
    }
  }
}

bool TrafficManager::IsEnabled() const
{
  return m_state != TrafficState::Disabled;
}

bool TrafficManager::IsInvalidState() const
{
  return m_state == TrafficState::NetworkError;
}

void TrafficManager::UpdateState()
{
  if (!IsEnabled() || IsInvalidState())
    return;

  auto const currentTime = steady_clock::now();
  auto maxPassedTime = steady_clock::duration::zero();

  bool waiting = false;
  bool networkError = false;
  bool expiredApp = false;
  bool expiredData = false;
  bool noData = false;

  for (auto const & mwmId : m_activeMwms)
  {
    auto it = m_mwmCache.find(mwmId);
    ASSERT(it != m_mwmCache.end(), ());

    if (it->second.m_isWaitingForResponse)
    {
      waiting = true;
    }
    else
    {
      expiredApp |= it->second.m_lastAvailability == traffic::TrafficInfo::Availability::ExpiredApp;
      expiredData |= it->second.m_lastAvailability == traffic::TrafficInfo::Availability::ExpiredData;
      noData |= it->second.m_lastAvailability == traffic::TrafficInfo::Availability::NoData;
    }

    if (it->second.m_isLoaded)
    {
      auto const timeSinceLastResponse = currentTime - it->second.m_lastResponseTime;
      if (timeSinceLastResponse > maxPassedTime)
        maxPassedTime = timeSinceLastResponse;
    }
    else if (it->second.m_retriesCount >= kMaxRetriesCount)
    {
      networkError = true;
    }
  }

  if (networkError || maxPassedTime >= kNetworkErrorTimeout)
    ChangeState(TrafficState::NetworkError);
  else if (waiting)
    ChangeState(TrafficState::WaitingData);
  else if (expiredApp)
    ChangeState(TrafficState::ExpiredApp);
  else if (expiredData)
    ChangeState(TrafficState::ExpiredData);
  else if (noData)
    ChangeState(TrafficState::NoData);
  else if (maxPassedTime >= kOutdatedDataTimeout)
    ChangeState(TrafficState::Outdated);
  else
    ChangeState(TrafficState::Enabled);
}

void TrafficManager::ChangeState(TrafficState newState)
{
  if (m_state == newState)
    return;

  m_state = newState;

  GetPlatform().RunOnGuiThread([this, newState]()
  {
    if (m_onStateChangedFn != nullptr)
      m_onStateChangedFn(newState);
  });
}
