#include "testing/testing.hpp"

#include "partners_api/yandex_api.hpp"

#include "geometry/latlon.hpp"

#include "base/scope_guard.hpp"

namespace
{
UNIT_TEST(Yandex_GetTaxiInfo)
{
  ms::LatLon const from(55.796918, 37.537859);
  ms::LatLon const to(55.758213, 37.616093);
  string result;

  taxi::yandex::RawApi::GetTaxiInfo(from, to, result);

  TEST(!result.empty(), ());
}

UNIT_TEST(Yandex_GetAvailableProducts)
{
  taxi::yandex::Api api;
  ms::LatLon const from(55.796918, 37.537859);
  ms::LatLon const to(55.758213, 37.616093);

  taxi::yandex::SetYandexUrlForTesting("http://localhost:34568/partners");
  MY_SCOPE_GUARD(cleanup, []() { taxi::yandex::SetYandexUrlForTesting(""); });

  std::vector<taxi::Product> resultProducts;

  api.GetAvailableProducts(from, to,
                           [&resultProducts](std::vector<taxi::Product> const & products) {
                             resultProducts = products;
                             testing::StopEventLoop();
                           },
                           [](taxi::ErrorCode const code) { TEST(false, ()); });

  testing::RunEventLoop();

  TEST(!resultProducts.empty(), ());
}

UNIT_TEST(Yandex_GetRideRequestLinks)
{
  taxi::yandex::Api api;
  ms::LatLon const from(55.796918, 37.537859);
  ms::LatLon const to(55.758213, 37.616093);

  auto const links = api.GetRideRequestLinks("", from, to);

  TEST(!links.m_deepLink.empty(), ());
}
}  // namespace
