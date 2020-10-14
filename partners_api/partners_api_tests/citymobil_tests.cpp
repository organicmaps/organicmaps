#include "testing/testing.hpp"

#include "partners_api/citymobil_api.hpp"

#include "geometry/latlon.hpp"

#include "platform/platform.hpp"

#include <string>

namespace
{
using Runner = Platform::ThreadRunner;

UNIT_TEST(Citymobil_GetSupportedTariffs)
{
  std::string result;
  taxi::citymobil::RawApi::SupportedTariffsBody supportedTariffs({55.796918, 37.537859});
  taxi::citymobil::RawApi::GetSupportedTariffs(supportedTariffs, result);

  TEST(!result.empty(), ());
}

UNIT_TEST(Citymobil_CalculatePrice)
{
  std::string result;
  taxi::citymobil::RawApi::CalculatePriceBody calculatePrice({55.796918, 37.537859},
                                                             {55.758213, 37.616093},
                                                             {1, 2, 3, 4, 5});
  taxi::citymobil::RawApi::CalculatePrice(calculatePrice, result);

  TEST(!result.empty(), ());
}

UNIT_CLASS_TEST(Runner, Citymobil_GetAvailableProducts)
{
  taxi::citymobil::Api api("http://localhost:34568/partners");
  ms::LatLon const from(55.796918, 37.537859);
  ms::LatLon const to(55.758213, 37.616093);

  std::vector<taxi::Product> resultProducts;

  api.GetAvailableProducts(from, to,
                           [&resultProducts](std::vector<taxi::Product> const & products) {
                             resultProducts = products;
                             testing::Notify();
                           },
                           [](taxi::ErrorCode const code) {
                             TEST(false, (code));
                           });

  testing::Wait();

  TEST(!resultProducts.empty(), ());

  taxi::ErrorCode errorCode = taxi::ErrorCode::RemoteError;
  ms::LatLon const farPos(56.838197, 35.908507);
  api.GetAvailableProducts(from, farPos,
                           [](std::vector<taxi::Product> const & products) {
                             TEST(false, ());
                           },
                           [&errorCode](taxi::ErrorCode const code) {
                             errorCode = code;
                             testing::Notify();
                           });

  testing::Wait();

  TEST_EQUAL(errorCode, taxi::ErrorCode::NoProducts, ());
}
}  // namespace
