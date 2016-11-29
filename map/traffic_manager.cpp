#include "map/traffic_manager.hpp"

#include "platform/platform.hpp"

#include "routing/routing_helpers.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

namespace
{
auto constexpr kUpdateInterval = minutes(1);
auto constexpr kOutdatedDataTimeout = minutes(5) + kUpdateInterval;
auto constexpr kNetworkErrorTimeout = minutes(20);

auto constexpr kMaxRetriesCount = 5;
} // namespace

TrafficManager::TrafficManager(GetMwmsByRectFn const & getMwmsByRectFn, size_t maxCacheSizeBytes)
  : m_getMwmsByRectFn(getMwmsByRectFn)
  , m_state(TrafficState::Disabled)
  , m_notifyStateChanged(false)
  , m_maxCacheSizeBytes(maxCacheSizeBytes)
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
    lock_guard<mutex> lock(m_requestedMwmsLock);
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

  NotifyStateChanged();
}

void TrafficManager::Clear()
{
  m_mwmCache.clear();
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

  {
    lock_guard<mutex> lock(m_requestedMwmsLock);
    Clear();
  }
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
  m_currentModelView = {screen, true};

  if (!IsEnabled() || IsInvalidState())
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
        m_activeMwms.insert(mwm);
    }

    RequestTrafficData();
  }

  NotifyStateChanged();
}

void TrafficManager::UpdateMyPosition(MyPosition const & myPosition)
{
  m_currentPosition = {myPosition, true};

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
        OnTrafficDataResponse(info);
      }
      else
      {
        LOG(LWARNING, ("Traffic request failed. Mwm =", mwm));
        OnTrafficRequestFailed(info);
      }

      NotifyStateChanged();
    }
    mwms.clear();
  }
}

bool TrafficManager::WaitForRequest(vector<MwmSet::MwmId> & mwms)
{
  unique_lock<mutex> lock(m_requestedMwmsLock);

  bool const timeout = !m_condition.wait_for(lock, kUpdateInterval, [this]
  {
    return !m_isRunning || !m_requestedMwms.empty();
  });

  if (!m_isRunning)
    return false;

  if (timeout && IsEnabled() && !IsInvalidState())
  {
    mwms.reserve(m_activeMwms.size());
    for (auto const & mwmId : m_activeMwms)
      mwms.push_back(mwmId);
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

    auto const currentTime = steady_clock::now();

    auto it = m_mwmCache.find(mwmId);
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
      RequestTrafficData(mwmId);
  }

  UpdateState();
}

void TrafficManager::RequestTrafficData(MwmSet::MwmId const & mwmId)
{
  m_requestedMwms.push_back(mwmId);
  m_condition.notify_one();
}

void TrafficManager::OnTrafficRequestFailed(traffic::TrafficInfo const & info)
{
  lock_guard<mutex> lock(m_requestedMwmsLock);

  auto it = m_mwmCache.find(info.GetMwmId());
  if (it == m_mwmCache.end())
    return;

  it->second.m_isWaitingForResponse = false;
  it->second.m_lastAvailability = info.GetAvailability();

  if (info.GetAvailability() == traffic::TrafficInfo::Availability::Unknown)
  {
    if (!it->second.m_isLoaded)
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
  }

  UpdateState();
}

void TrafficManager::OnTrafficDataResponse(traffic::TrafficInfo const & info)
{
  lock_guard<mutex> lock(m_requestedMwmsLock);

  auto it = m_mwmCache.find(info.GetMwmId());
  if (it == m_mwmCache.end())
    return;

  it->second.m_isLoaded = true;
  it->second.m_lastResponseTime = steady_clock::now();
  it->second.m_isWaitingForResponse = false;
  it->second.m_lastAvailability = info.GetAvailability();

  // Update cache.
  size_t constexpr kElementSize = sizeof(traffic::TrafficInfo::RoadSegmentId) + sizeof(traffic::SpeedGroup);
  size_t const dataSize = info.GetColoring().size() * kElementSize;
  m_currentCacheSizeBytes += (dataSize - it->second.m_dataSize);
  it->second.m_dataSize = dataSize;
  CheckCacheSize();

  // Update traffic colors.
  df::TrafficSegmentsColoring coloring;
  coloring[info.GetMwmId()] = info.GetColoring();
  m_drapeEngine->UpdateTraffic(coloring);

  UpdateState();
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
        m_currentCacheSizeBytes -= it->second.m_dataSize;
        m_drapeEngine->ClearTrafficCache(mwmId);
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
  bool expiredMwm = false;
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
      expiredMwm |= it->second.m_lastAvailability == traffic::TrafficInfo::Availability::ExpiredMwm;
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
  else if (expiredMwm)
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
  m_notifyStateChanged = true;
}

void TrafficManager::NotifyStateChanged()
{
  if (m_notifyStateChanged)
  {
    m_notifyStateChanged = false;

    GetPlatform().RunOnGuiThread([this]()
    {
      if (m_onStateChangedFn != nullptr)
        m_onStateChangedFn(m_state);
    });
  }
}
