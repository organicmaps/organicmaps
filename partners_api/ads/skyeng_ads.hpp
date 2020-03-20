#pragma once

#include "partners_api/ads/ads_base.hpp"

namespace ads
{
class Skyeng : public DownloadOnMapContainer
{
public:
  explicit Skyeng(Delegate & delegate);

private:
  std::string GetBannerInternal() const override;
};
}  // namespace ads
