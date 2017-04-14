#pragma once

#include "partners_api/ads_base.hpp"

namespace ads
{
// Class which matches feature types and mopub banner ids.
class Mopub : public Container
{
public:
  Mopub();

  // ContainerBase overrides:
  bool HasSearchBanner() const override;
  std::string GetSearchBannerId() const override;
  std::string GetBannerIdForOtherTypes() const override;
};
}  // namespace ads
