#pragma once

#include "partners_api/ads/ads_base.hpp"

namespace ads
{
// Class which matches feature types and RB banner ids.
class Rb : public PoiContainer
{
public:
  Rb();

private:
  // PoiContainerBase overrides:
  std::string GetBannerForOtherTypes() const override;
};
}  // namespace ads
