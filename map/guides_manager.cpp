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
