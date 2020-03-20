#pragma once

#include "partners_api/ads/ads_base.hpp"

namespace ads
{
class TinkoffInsurance : public DownloadOnMapContainer
{
public:
  explicit TinkoffInsurance(Delegate & delegate);

private:
  std::string GetBannerInternal() const override;
};
}  // namespace ads
