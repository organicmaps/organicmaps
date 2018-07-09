#pragma once

#include "partners_api/ads_base.hpp"

namespace ads
{
// Class which matches feature types and RB banner ids.
class Rb : public Container
{
public:
  Rb();

  // ContainerBase overrides:
  std::string GetBannerIdForOtherTypes() const override;
};
}  // namespace ads
