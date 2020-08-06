#pragma once

#include "partners_api/ads/ads_base.hpp"

namespace ads
{
class MastercardSberbank : public DownloadOnMapContainer
{
public:
  explicit  MastercardSberbank(Delegate & delegate);

private:
  std::string GetBannerInternal() const override;
};
}  // namespace ads