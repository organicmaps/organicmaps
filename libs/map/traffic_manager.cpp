#include "map/traffic_manager.hpp"

#include "routing/routing_helpers.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

using namespace std::chrono;

namespace
{
auto constexpr kUpdateInterval = minutes(1);
auto constexpr kOutdatedDataTimeout = minutes(5) + kUpdateInterval;
auto constexpr kNetworkErrorTimeout = minutes(20);

auto constexpr kMaxRetriesCount = 5;
}  // namespace

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
  , m_lastActiveTime(requestTime)
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
  , m_isPaused(false)
  , m_thread(&TrafficManager::ThreadRoutine, this)
{
  CHECK(m_getMwmsByRectFn != nullptr, ());
}

TrafficManager::~TrafficManager()
{
#ifdef DEBUG
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    ASSERT(!m_isRunning, ());
  }
#endif
}

void TrafficManager::Teardown()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isRunning)
      return;
    m_isRunning = false;
  }
  m_condition.notify_one();
  m_thread.join();
}

TrafficManager::TrafficState TrafficManager::GetState() const
{
  return m_state;
}

void TrafficManager::SetStateListener(TrafficStateChangedFn const & onStateChangedFn)
{
  m_onStateChangedFn = onStateChangedFn;
}

void TrafficManager::SetEnabled(bool enabled)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (enabled == IsEnabled())
      return;
    Clear();
    ChangeState(enabled ? TrafficState::Enabled : TrafficState::Disabled);
  }

  m_drapeEngine.SafeCall(&df::DrapeEngine::EnableTraffic, enabled);

  if (enabled)
    Invalidate();
  else
    m_observer.OnTrafficInfoClear();
}

void TrafficManager::Clear()
{
  m_currentCacheSizeBytes = 0;
  m_mwmCache.clear();
  m_lastDrapeMwmsByRect.clear();
  m_lastRoutingMwmsByRect.clear();
  m_activeDrapeMwms.clear();
  m_activeRoutingMwms.clear();
  m_requestedMwms.clear();
  m_trafficETags.clear();
}

void TrafficManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
}

void TrafficManager::SetCurrentDataVersion(int64_t dataVersion)
{
  m_currentDataVersion = dataVersion;
}

void TrafficManager::OnMwmDeregistered(platform::LocalCountryFile const & countryFile)
{
  if (!IsEnabled())
    return;

  {
    std::lock_guard<std::mutex> lock(m_mutex);

    MwmSet::MwmId mwmId;
    for (auto const & cacheEntry : m_mwmCache)
    {
      if (cacheEntry.first.IsDeregistered(countryFile))
      {
        mwmId = cacheEntry.first;
        break;
      }
    }

    ClearCache(mwmId);
  }
}

void TrafficManager::OnDestroySurface()
{
  Pause();
}

void TrafficManager::OnRecoverSurface()
{
  Resume();
}

void TrafficManager::Invalidate()
{
  if (!IsEnabled())
    return;

  m_lastDrapeMwmsByRect.clear();
  m_lastRoutingMwmsByRect.clear();

  if (m_currentModelView.second)
    UpdateViewport(m_currentModelView.first);
  if (m_currentPosition.second)
    UpdateMyPosition(m_currentPosition.first);
}

void TrafficManager::UpdateActiveMwms(m2::RectD const & rect, std::vector<MwmSet::MwmId> & lastMwmsByRect,
                                      std::set<MwmSet::MwmId> & activeMwms)
{
  auto mwms = m_getMwmsByRectFn(rect);
  if (lastMwmsByRect == mwms)
    return;
  lastMwmsByRect = mwms;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    activeMwms.clear();
    for (auto const & mwm : mwms)
      if (mwm.IsAlive())
        activeMwms.insert(mwm);
    RequestTrafficData();
  }
}

void TrafficManager::UpdateMyPosition(MyPosition const & myPosition)
{
  // Side of square around |myPosition|. Every mwm which is covered by the square
  // will get traffic info.
  double const kSquareSideM = 5000.0;
  m_currentPosition = {myPosition, true /* initialized */};

  if (!IsEnabled() || IsInvalidState() || m_isPaused)
    return;

  m2::RectD const rect = mercator::RectByCenterXYAndSizeInMeters(myPosition.m_position, kSquareSideM / 2.0);
  // Request traffic.
  UpdateActiveMwms(rect, m_lastRoutingMwmsByRect, m_activeRoutingMwms);

  // @TODO Do all routing stuff.
}

void TrafficManager::UpdateViewport(ScreenBase const & screen)
{
  m_currentModelView = {screen, true /* initialized */};

  if (!IsEnabled() || IsInvalidState() || m_isPaused)
    return;

  if (df::GetZoomLevel(screen.GetScale()) < df::kRoadClass0ZoomLevel)
    return;

  // Request traffic.
  UpdateActiveMwms(screen.ClipRect(), m_lastDrapeMwmsByRect, m_activeDrapeMwms);
}

void TrafficManager::ThreadRoutine()
{
  std::vector<MwmSet::MwmId> mwms;
  while (WaitForRequest(mwms))
  {
    for (auto const & mwm : mwms)
    {
      if (!mwm.IsAlive())
        continue;

      traffic::TrafficInfo info(mwm, m_currentDataVersion);

      std::string tag;
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        tag = m_trafficETags[mwm];
      }

      if (info.ReceiveTrafficData(tag))
      {
        OnTrafficDataResponse(std::move(info));
      }
      else
      {
        LOG(LWARNING, ("Traffic request failed. Mwm =", mwm));
        OnTrafficRequestFailed(std::move(info));
      }

      {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_trafficETags[mwm] = tag;
      }
    }
    mwms.clear();
  }
}

bool TrafficManager::WaitForRequest(std::vector<MwmSet::MwmId> & mwms)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  bool const timeout =
      !m_condition.wait_for(lock, kUpdateInterval, [this] { return !m_isRunning || !m_requestedMwms.empty(); });

  if (!m_isRunning)
    return false;

  if (timeout)
    RequestTrafficData();

  if (!m_requestedMwms.empty())
    mwms.swap(m_requestedMwms);

  return true;
}

void TrafficManager::RequestTrafficData(MwmSet::MwmId const & mwmId, bool force)
{
  bool needRequesting = false;
  auto const currentTime = steady_clock::now();
  auto const it = m_mwmCache.find(mwmId);
  if (it == m_mwmCache.end())
  {
    needRequesting = true;
    m_mwmCache.insert(std::make_pair(mwmId, CacheEntry(currentTime)));
  }
  else
  {
    auto const passedSeconds = currentTime - it->second.m_lastRequestTime;
    if (passedSeconds >= kUpdateInterval || force)
    {
      needRequesting = true;
      it->second.m_isWaitingForResponse = true;
      it->second.m_lastRequestTime = currentTime;
    }
    if (!force)
      it->second.m_lastActiveTime = currentTime;
  }

  if (needRequesting)
  {
    m_requestedMwms.push_back(mwmId);
    m_condition.notify_one();
  }
}

void TrafficManager::RequestTrafficData()
{
  if ((m_activeDrapeMwms.empty() && m_activeRoutingMwms.empty()) || !IsEnabled() || IsInvalidState() || m_isPaused)
    return;

  ForEachActiveMwm([this](MwmSet::MwmId const & mwmId)
  {
    ASSERT(mwmId.IsAlive(), ());
    RequestTrafficData(mwmId, false /* force */);
  });
  UpdateState();
}

void TrafficManager::OnTrafficRequestFailed(traffic::TrafficInfo && info)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_mwmCache.find(info.GetMwmId());
  if (it == m_mwmCache.end())
    return;

  it->second.m_isWaitingForResponse = false;
  it->second.m_lastAvailability = info.GetAvailability();

  if (info.GetAvailability() == traffic::TrafficInfo::Availability::Unknown && !it->second.m_isLoaded)
  {
    if (m_activeDrapeMwms.find(info.GetMwmId()) != m_activeDrapeMwms.cend() ||
        m_activeRoutingMwms.find(info.GetMwmId()) != m_activeRoutingMwms.cend())
    {
      if (it->second.m_retriesCount < kMaxRetriesCount)
        RequestTrafficData(info.GetMwmId(), true /* force */);
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
    std::lock_guard<std::mutex> lock(m_mutex);

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
      ShrinkCacheToAllowableSize();
    }

    UpdateState();
  }

  if (!info.GetColoring().empty())
  {
    m_drapeEngine.SafeCall(&df::DrapeEngine::UpdateTraffic, static_cast<traffic::TrafficInfo const &>(info));

    // Update traffic colors for routing.
    m_observer.OnTrafficInfoAdded(std::move(info));
  }
}

void TrafficManager::UniteActiveMwms(std::set<MwmSet::MwmId> & activeMwms) const
{
  activeMwms.insert(m_activeDrapeMwms.cbegin(), m_activeDrapeMwms.cend());
  activeMwms.insert(m_activeRoutingMwms.cbegin(), m_activeRoutingMwms.cend());
}

void TrafficManager::ShrinkCacheToAllowableSize()
{
  // Calculating number of different active mwms.
  std::set<MwmSet::MwmId> activeMwms;
  UniteActiveMwms(activeMwms);
  size_t const numActiveMwms = activeMwms.size();

  if (m_currentCacheSizeBytes > m_maxCacheSizeBytes && m_mwmCache.size() > numActiveMwms)
  {
    std::multimap<time_point<steady_clock>, MwmSet::MwmId> seenTimings;
    for (auto const & mwmInfo : m_mwmCache)
      seenTimings.insert(std::make_pair(mwmInfo.second.m_lastActiveTime, mwmInfo.first));

    auto itSeen = seenTimings.begin();
    while (m_currentCacheSizeBytes > m_maxCacheSizeBytes && m_mwmCache.size() > numActiveMwms)
    {
      ClearCache(itSeen->second);
      ++itSeen;
    }
  }
}

void TrafficManager::ClearCache(MwmSet::MwmId const & mwmId)
{
  auto const it = m_mwmCache.find(mwmId);
  if (it == m_mwmCache.end())
    return;

  if (it->second.m_isLoaded)
  {
    ASSERT_GREATER_OR_EQUAL(m_currentCacheSizeBytes, it->second.m_dataSize, ());
    m_currentCacheSizeBytes -= it->second.m_dataSize;

    m_drapeEngine.SafeCall(&df::DrapeEngine::ClearTrafficCache, mwmId);

    GetPlatform().RunTask(Platform::Thread::Gui, [this, mwmId]() { m_observer.OnTrafficInfoRemoved(mwmId); });
  }
  m_mwmCache.erase(it);
  m_trafficETags.erase(mwmId);
  m_activeDrapeMwms.erase(mwmId);
  m_activeRoutingMwms.erase(mwmId);
  m_lastDrapeMwmsByRect.clear();
  m_lastRoutingMwmsByRect.clear();
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

  for (MwmSet::MwmId const & mwmId : m_activeDrapeMwms)
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

  GetPlatform().RunTask(Platform::Thread::Gui, [this, newState]()
  {
    if (m_onStateChangedFn != nullptr)
      m_onStateChangedFn(newState);
  });
}

void TrafficManager::OnEnterForeground()
{
  Resume();
}

void TrafficManager::OnEnterBackground()
{
  Pause();
}

void TrafficManager::Pause()
{
  m_isPaused = true;
}

void TrafficManager::Resume()
{
  if (!m_isPaused)
    return;

  m_isPaused = false;
  Invalidate();
}

void TrafficManager::SetSimplifiedColorScheme(bool simplified)
{
  m_hasSimplifiedColorScheme = simplified;
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetSimplifiedTrafficColors, simplified);
}

std::string DebugPrint(TrafficManager::TrafficState state)
{
  switch (state)
  {
  case TrafficManager::TrafficState::Disabled: return "Disabled";
  case TrafficManager::TrafficState::Enabled: return "Enabled";
  case TrafficManager::TrafficState::WaitingData: return "WaitingData";
  case TrafficManager::TrafficState::Outdated: return "Outdated";
  case TrafficManager::TrafficState::NoData: return "NoData";
  case TrafficManager::TrafficState::NetworkError: return "NetworkError";
  case TrafficManager::TrafficState::ExpiredData: return "ExpiredData";
  case TrafficManager::TrafficState::ExpiredApp: return "ExpiredApp";
  default: ASSERT(false, ("Unknown state"));
  }
  return "Unknown";
}
