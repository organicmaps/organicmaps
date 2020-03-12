#pragma once

#include "partners_api/ads_base.hpp"

#include <string>

namespace ads
{
// Class which matches feature types and mopub banner ids.
class Mopub : public PoiContainer
{
public:
  Mopub();

  // PoiContainerBase overrides:
  std::string GetBannerForOtherTypes() const override;

  static std::string InitializationBannerId();
};
}  // namespace ads
