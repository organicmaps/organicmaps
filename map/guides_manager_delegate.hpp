#pragma once

#include "map/framework.hpp"
#include "map/guides_manager.hpp"

class GuidesManagerDelegate : public GuidesManager::Delegate
{
public:
  explicit GuidesManagerDelegate(Framework const & framework);

  bool IsGuideDownloaded(std::string const & guideId) const override;

private:
  Framework const & m_framework;
};
