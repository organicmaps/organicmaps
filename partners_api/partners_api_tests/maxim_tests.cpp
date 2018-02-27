#include "testing/testing.hpp"

#include "partners_api/maxim_api.hpp"

#include "geometry/latlon.hpp"

#include "platform/platform.hpp"

namespace
{
string const kTestResponse = R"(
{
  "ButtonText": null,
  "Message": null,
  "PaymentMethods": null,
  "Prices": null,
  "ShowFrom": false,
  "Success": true,
  "TypeId": 2,
  "CurrencyCode": "RUB",
  "FeedTime": 9,
  "Price": 244,
  "PriceString": "244.00 ₽"
}
)";

using Runner = Platform::ThreadRunner;

UNIT_TEST(Maxim_GetTaxiInfo)
{
  ms::LatLon const from(55.796918, 37.537859);
  ms::LatLon const to(55.758213, 37.616093);
  string result;

  taxi::maxim::RawApi::GetTaxiInfo(from, to, result);

  TEST(!result.empty(), ());
}

UNIT_TEST(Maxim_MakeFromJson)
{
  std::vector<taxi::Product> products;

  taxi::maxim::MakeFromJson(kTestResponse, products);

  TEST(!products.empty(), ());
  TEST_EQUAL(products.size(), 1, ());
  TEST_EQUAL(products[0].m_price, "244.00 ₽", ());
  TEST_EQUAL(products[0].m_time, "540", ());
  TEST_EQUAL(products[0].m_currency, "RUB", ());
}

UNIT_CLASS_TEST(Runner, Maxim_GetAvailableProducts)
{
  taxi::maxim::Api api("http://localhost:34568/partners");
  ms::LatLon const from(55.796918, 37.537859);
  ms::LatLon const to(55.758213, 37.616093);

  std::vector<taxi::Product> resultProducts;

  api.GetAvailableProducts(from, to,
                           [&resultProducts](std::vector<taxi::Product> const & products) {
                             resultProducts = products;
                             testing::Notify();
                           },
                           [](taxi::ErrorCode const code) { TEST(false, (code)); });

  testing::Wait();

  TEST(!resultProducts.empty(), ());

  taxi::ErrorCode errorCode = taxi::ErrorCode::RemoteError;
  ms::LatLon const farPos(56.838197, 35.908507);
  api.GetAvailableProducts(from, farPos,
                           [](std::vector<taxi::Product> const & products) { TEST(false, ()); },
                           [&errorCode](taxi::ErrorCode const code) {
                             errorCode = code;
                             testing::Notify();
                           });

  testing::Wait();

  TEST_EQUAL(errorCode, taxi::ErrorCode::NoProducts, ());
}

UNIT_CLASS_TEST(Runner, Maxim_NoTaxi)
{
  taxi::maxim::Api api;
  taxi::ErrorCode errorCode = taxi::ErrorCode::RemoteError;
  ms::LatLon const noTaxiFrom(-66.876016, 53.241745);
  ms::LatLon const noTaxiTo(-66.878029, 53.323667);
  api.GetAvailableProducts(noTaxiFrom, noTaxiTo,
                           [](std::vector<taxi::Product> const & products) { TEST(false, ()); },
                           [&errorCode](taxi::ErrorCode const code) {
                             errorCode = code;
                             testing::Notify();
                           });

  testing::Wait();
  TEST_EQUAL(errorCode, taxi::ErrorCode::NoProducts, ());
}
}  // namespace
