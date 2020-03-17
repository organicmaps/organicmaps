#pragma once

#include "partners_api/ads/ads_base.hpp"

namespace ads
{
class TinkoffAllAirlines : public DownloadOnMapContainer
{
public:
  TinkoffAllAirlines(Delegate & delegate);

private:
  std::string GetBannerInternal() const override;
};
}  // namespace ads
