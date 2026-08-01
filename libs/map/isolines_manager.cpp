#include "map/isolines_manager.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/terrain/terrain_utils.hpp"

#include "base/assert.hpp"

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

void IsolinesManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
}

void IsolinesManager::SetEnabled(bool enabled)
{
  ChangeState(enabled ? IsolinesState::Enabled : IsolinesState::Disabled);
  m_drapeEngine.SafeCall(&df::DrapeEngine::EnableIsolines, enabled);
  if (enabled)
    Invalidate();
}

bool IsolinesManager::IsEnabled() const
{
  return m_state != IsolinesState::Disabled;
}

bool IsolinesManager::IsVisible() const
{
  return m_currentModelView && df::GetDrawTileScale(*m_currentModelView) >= terrain::kMinIsolinesZoom;
}

void IsolinesManager::UpdateViewport(ScreenBase const & screen)
{
  if (screen.GlobalRect().GetLocalRect().IsEmptyInterior())
    return;

  m_currentModelView = screen;
  if (!IsEnabled())
    return;

  // Keep the last state on the low zooms: the platforms announce the NoData transition
  // (the terrain download hint), don't repeat it on every zoom bounce over the same place.
  if (!IsVisible())
    return;

  bool const hasTerrain = m_hasTerrainFn && m_hasTerrainFn(screen.ClipRect());
  ChangeState(hasTerrain ? IsolinesState::Enabled : IsolinesState::NoData);
}

void IsolinesManager::Invalidate()
{
  if (!IsEnabled())
    return;
  if (m_currentModelView)
    UpdateViewport(*m_currentModelView);
}

std::string DebugPrint(IsolinesManager::IsolinesState state)
{
  switch (state)
  {
  case IsolinesManager::IsolinesState::Disabled: return "Disabled";
  case IsolinesManager::IsolinesState::Enabled: return "Enabled";
  case IsolinesManager::IsolinesState::NoData: return "NoData";
  }
  UNREACHABLE();
}
