#pragma once

#include "partners_api/ads/ads_base.hpp"

namespace ads
{
class Citymobil : public SearchCategoryContainerBase
{
public:
  explicit Citymobil(Delegate & delegate);

  // SearchCategoryContainerBase overrides:
  std::string GetBanner(std::optional<m2::PointD> const & userPos) const override;
};
}  // namespace ads
