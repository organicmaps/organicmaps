#include "map/guides_manager_delegate.hpp"

GuidesManagerDelegate::GuidesManagerDelegate(Framework const & framework)
  : m_framework(framework)
{
}

bool GuidesManagerDelegate::IsGuideDownloaded(std::string const & guideId) const
{
  return m_framework.GetBookmarkManager().GetCatalog().HasDownloaded(guideId);
}
