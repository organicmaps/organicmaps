#pragma once

#include "partners_api/ads/ads_base.hpp"

namespace ads
{
class Mts : public DownloadOnMapContainer
{
public:
  Mts(Delegate & delegate);

private:
  std::string GetBannerInternal() const override;
};
}  // namespace ads
