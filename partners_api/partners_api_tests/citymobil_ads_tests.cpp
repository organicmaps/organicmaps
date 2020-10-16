#include "testing/testing.hpp"

#include "partners_api/ads/citymobil_ads.hpp"

namespace
{
class DelegateForTesting : public ads::SearchCategoryContainerBase::Delegate
{
public:
  // ads::SearchCategoryContainerBase::Delegate overrides.
  std::vector<taxi::Provider::Type> GetTaxiProvidersAtPos(m2::PointD const & pos) const override
  {
    return m_providers;
  }

  void SetProviders(std::vector<taxi::Provider::Type> const & providers)
  {
    m_providers = providers;
  }

private:
  std::vector<taxi::Provider::Type> m_providers;
};

UNIT_TEST(Citymobil_GetBanner)
{
  DelegateForTesting delegate;
  ads::Citymobil citymobil(delegate);
  m2::PointD point;

  {
    auto const banner = citymobil.GetBanner(point);
    TEST(banner.empty(), ());
  }
  {
    delegate.SetProviders({taxi::Provider::Type::Uber, taxi::Provider::Type::Freenow});
    auto const banner = citymobil.GetBanner(point);
    TEST(banner.empty(), ());
  }
  {
    delegate.SetProviders({taxi::Provider::Type::Uber, taxi::Provider::Type::Citymobil});
    auto const banner = citymobil.GetBanner(point);
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetProviders({taxi::Provider::Type::Citymobil, taxi::Provider::Type::Uber});
    auto const banner = citymobil.GetBanner(point);
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetProviders({taxi::Provider::Type::Citymobil});
    auto const banner = citymobil.GetBanner(point);
    TEST(!banner.empty(), ());
  }
}
}  // namespace
