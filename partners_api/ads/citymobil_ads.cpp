#include "partners_api/ads/citymobil_ads.hpp"

namespace ads
{
Citymobil::Citymobil(Delegate & delegate)
  : SearchCategoryContainerBase(delegate)
{
}

std::string Citymobil::GetBanner(std::optional<m2::PointD> const & userPos) const
{
  if (!userPos)
    return {};

  auto const providers = m_delegate.GetTaxiProvidersAtPos(*userPos);
  if (std::find(providers.begin(), providers.end(), taxi::Provider::Type::Citymobil) == providers.end())
    return {};

  return "https://trk.mail.ru/c/oimab9";
}
}  // namespace ads
