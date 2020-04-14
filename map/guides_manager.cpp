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

bool GuidesManager::GetGallery(std::string const & guideId, std::vector<GuideInfo> & guidesInfo)
{
  // TODO: Implement.
}

void GuidesManager::SetActiveGuide(std::string const & guideId)
{
  // TODO: Implement.
}
