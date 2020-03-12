#pragma once

#include "partners_api/ads_base.hpp"

namespace ads
{
// Class which matches feature types and facebook banner ids.
class Google : public SearchContainerBase
{
public:
  // PoiContainerBase overrides:
  bool HasBanner() const override;
  std::string GetBanner() const override;
};
}  // namespace ads
