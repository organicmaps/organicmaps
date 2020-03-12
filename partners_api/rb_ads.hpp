#pragma once

#include "partners_api/ads_base.hpp"

namespace ads
{
// Class which matches feature types and RB banner ids.
class Rb : public PoiContainer
{
public:
  Rb();

  // PoiContainerBase overrides:
  std::string GetBannerForOtherTypes() const override;
};
}  // namespace ads
