#pragma once

#include "partners_api/ads/ads_base.hpp"

namespace ads
{
// Class which matches feature types and facebook banner ids.
class Google : public SearchContainerBase
{
public:
  // SearchContainerBase overrides:
  std::string GetBanner() const override;

private:
  // SearchContainerBase overrides:
  bool HasBanner() const override;
};
}  // namespace ads
