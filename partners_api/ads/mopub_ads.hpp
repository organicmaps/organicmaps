#pragma once

#include "partners_api/ads/ads_base.hpp"

#include <string>

namespace ads
{
// Class which matches feature types and mopub banner ids.
class Mopub : public PoiContainer
{
public:
  Mopub();

  static std::string InitializationBannerId();

private:
  // PoiContainerBase overrides:
  std::string GetBannerForOtherTypes() const override;
};
}  // namespace ads
