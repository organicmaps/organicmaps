#include "map/guides_manager.hpp"

GuidesManager::GuidesState GuidesManager::GetState() const
{
  return m_state;
}

void GuidesManager::SetStateListener(GuidesStateChangedFn const & onStateChangedFn)
{
  m_onStateChangedFn = onStateChangedFn;
}

void GuidesManager::UpdateViewport(ScreenBase const & screen)
{
  // TODO: Implement.
}

void GuidesManager::Invalidate()
{
  // TODO: Implement.
}

void GuidesManager::SetEnabled(bool enabled)
{
  ChangeState(enabled ? GuidesState::Enabled : GuidesState::Disabled);
}

bool GuidesManager::IsEnabled() const
{
  return m_state != GuidesState::Disabled;
}

void GuidesManager::ChangeState(GuidesState newState)
{
  if (m_state == newState)
    return;
  m_state = newState;
  if (m_onStateChangedFn != nullptr)
    m_onStateChangedFn(newState);
}

GuidesManager::GuidesGallery GuidesManager::GetGallery() const
{
  // TODO: Implement.
  return {};
}

std::string GuidesManager::GetActiveGuide() const
{
  // TODO: Implement.
  return "";
}

void GuidesManager::SetActiveGuide(std::string const & guideId)
{
  // TODO: Implement.
}

void GuidesManager::SetGalleryListener(GuidesGalleryChangedFn const & onGalleryChangedFn)
{
  m_onGalleryChangedFn = onGalleryChangedFn;
}

void GuidesManager::SetApiDelegate(std::unique_ptr<GuidesOnMapDelegate> apiDelegate)
{
  m_api.SetDelegate(std::move(apiDelegate));
}
