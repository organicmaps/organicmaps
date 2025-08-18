#include "map/isolines_manager.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/mwm_traits.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

int constexpr kMinIsolinesZoom = 11;

IsolinesManager::IsolinesManager(DataSource & dataSource, GetMwmsByRectFn const & getMwmsByRectFn)
  : m_dataSource(dataSource)
  , m_getMwmsByRectFn(getMwmsByRectFn)
{
  CHECK(m_getMwmsByRectFn != nullptr, ());
}

IsolinesManager::IsolinesState IsolinesManager::GetState() const
{
  return m_state;
}

void IsolinesManager::SetStateListener(IsolinesStateChangedFn const & onStateChangedFn)
{
  m_onStateChangedFn = onStateChangedFn;
}

void IsolinesManager::ChangeState(IsolinesState newState)
{
  if (m_state == newState)
    return;
  m_state = newState;
  if (m_onStateChangedFn != nullptr)
    m_onStateChangedFn(newState);
}

IsolinesManager::Info const & IsolinesManager::LoadIsolinesInfo(MwmSet::MwmId const & id) const
{
  Availability status = Availability::NoData;
  isolines::Quality quality = isolines::Quality::None;
  isolines::IsolinesInfo info;
  if (isolines::LoadIsolinesInfo(m_dataSource, id, info))
  {
    LOG(LINFO, ("Isolines min altitude", info.m_minAltitude, "max altitude", info.m_maxAltitude, "altitude step",
                info.m_altStep));
    status = Availability::Available;
    quality = info.GetQuality();
  }

  return m_mwmCache.emplace(id, Info(status, quality)).first->second;
}

void IsolinesManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
}

void IsolinesManager::SetEnabled(bool enabled)
{
  ChangeState(enabled ? IsolinesState::Enabled : IsolinesState::Disabled);
  m_drapeEngine.SafeCall(&df::DrapeEngine::EnableIsolines, enabled);
  if (enabled)
  {
    Invalidate();
  }
  else
  {
    m_lastMwms.clear();
    m_mwmCache.clear();
  }
}

bool IsolinesManager::IsEnabled() const
{
  return m_state != IsolinesState::Disabled;
}

bool IsolinesManager::IsVisible() const
{
  return m_currentModelView && df::GetDrawTileScale(*m_currentModelView) >= kMinIsolinesZoom;
}

void IsolinesManager::UpdateViewport(ScreenBase const & screen)
{
  if (screen.GlobalRect().GetLocalRect().IsEmptyInterior())
    return;

  m_currentModelView = screen;
  if (!IsEnabled())
    return;

  if (!IsVisible())
  {
    ChangeState(IsolinesState::Enabled);
    return;
  }

  auto mwms = m_getMwmsByRectFn(screen.ClipRect());
  if (m_lastMwms == mwms)
    return;
  m_lastMwms = std::move(mwms);
  for (auto const & mwmId : m_lastMwms)
  {
    if (!mwmId.IsAlive())
      continue;
    auto it = m_mwmCache.find(mwmId);
    if (it == m_mwmCache.end())
      LoadIsolinesInfo(mwmId);
  }
  UpdateState();
}

void IsolinesManager::UpdateState()
{
  bool available = false;
  bool expired = false;
  bool noData = false;
  for (auto const & mwmId : m_lastMwms)
  {
    if (!mwmId.IsAlive())
      continue;
    auto const it = m_mwmCache.find(mwmId);
    CHECK(it != m_mwmCache.end(), ());
    switch (it->second.m_availability)
    {
    case Availability::Available: available = true; break;
    case Availability::ExpiredData: expired = true; break;
    case Availability::NoData: noData = true; break;
    }
  }

  if (expired)
    ChangeState(IsolinesState::ExpiredData);
  else if (!available && noData)
    ChangeState(IsolinesState::NoData);
  else
    ChangeState(IsolinesState::Enabled);
}

void IsolinesManager::Invalidate()
{
  if (!IsEnabled())
    return;
  m_lastMwms.clear();
  if (m_currentModelView)
    UpdateViewport(*m_currentModelView);
}

isolines::Quality IsolinesManager::GetDataQuality(MwmSet::MwmId const & id) const
{
  if (!id.IsAlive())
    return isolines::Quality::None;

  auto const it = m_mwmCache.find(id);
  if (it == m_mwmCache.cend())
    return LoadIsolinesInfo(id).m_quality;

  return it->second.m_quality;
}

void IsolinesManager::OnMwmDeregistered(platform::LocalCountryFile const & countryFile)
{
  for (auto it = m_mwmCache.begin(); it != m_mwmCache.end(); ++it)
  {
    if (it->first.IsDeregistered(countryFile))
    {
      m_mwmCache.erase(it);
      break;
    }
  }
}

std::string DebugPrint(IsolinesManager::IsolinesState state)
{
  switch (state)
  {
  case IsolinesManager::IsolinesState::Disabled: return "Disabled";
  case IsolinesManager::IsolinesState::Enabled: return "Enabled";
  case IsolinesManager::IsolinesState::ExpiredData: return "ExpiredData";
  case IsolinesManager::IsolinesState::NoData: return "NoData";
  }
  UNREACHABLE();
}
