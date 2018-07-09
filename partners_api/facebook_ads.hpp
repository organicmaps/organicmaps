#pragma once

#include "partners_api/ads_base.hpp"

namespace ads
{
// Class which matches feature types and facebook banner ids.
class Facebook : public Container
{
public:
  Facebook() = default;

  // ContainerBase overrides:
  bool HasSearchBanner() const override;
  std::string GetBannerIdForOtherTypes() const override;
  std::string GetSearchBannerId() const override;
};
}  // namespace ads
