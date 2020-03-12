#pragma once

#include "partners_api/ads_base.hpp"

namespace ads
{
// Class which matches feature types and facebook banner ids.
class FacebookPoi : public PoiContainer
{
public:
  // PoiContainerBase overrides:
  std::string GetBannerForOtherTypes() const override;
};

class FacebookSearch : public SearchContainerBase
{
public:
  // SearchContainerBase overrides:
  bool HasBanner() const override;
  std::string GetBanner() const override;
};
}  // namespace ads
