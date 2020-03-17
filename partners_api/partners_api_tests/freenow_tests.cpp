#include "testing/testing.hpp"

#include "partners_api/freenow_api.hpp"

#include "geometry/latlon.hpp"

#include "platform/platform.hpp"

#include <string>

namespace
{
using Runner = Platform::ThreadRunner;

std::string const kTokenResponse = R"(
{
  "access_token": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXV",
  "token_type": "bearer",
  "expires_in": 600,
  "scope": "service-types"
})";

std::string const kServiceTypesResponse = R"(
{
  "serviceTypes": [
    {
      "id": "TAXI",
      "type": "TAXI",
      "displayName": "Taxi",
      "eta": {
        "value": 0,
        "displayValue": "0 Minutes"
      },
      "fare": {
        "type": "FIXED",
        "value": 5000,
        "currencyCode": "GBP",
        "displayValue": "5000GBP"
      },
      "availablePaymentMethodTypes": [
        "BUSINESS_ACCOUNT",
        "CREDIT_CARD",
        "PAYPAL",
        "CASH"
      ],
      "seats": {
        "max": 4,
        "values": [],
        "displayValue": "4"
      },
      "availableBookingOptions": [
        {
          "name": "COMMENT",
          "displayName": "COMMENT",
          "type": "TEXT"
        },
        {
          "name": "MERCEDES",
          "displayName": "MERCEDES",
          "type": "BOOLEAN"
        },
        {
          "name": "FAVORITE_DRIVER",
          "displayName": "FAVORITE_DRIVER",
          "type": "BOOLEAN"
        },
        {
          "name": "FIVE_STARS",
          "displayName": "FIVE_STARS",
          "type": "BOOLEAN"
        },
        {
          "name": "SMALL_ANIMAL",
          "displayName": "SMALL_ANIMAL",
          "type": "BOOLEAN"
        }
      ]
    }
  ]
})";

UNIT_TEST(Freenow_GetAccessToken)
{
  ms::LatLon const from(55.796918, 37.537859);
  ms::LatLon const to(55.758213, 37.616093);
  std::string result;

  taxi::freenow::RawApi::GetAccessToken(result);
  TEST(!result.empty(), ());

  auto const token = taxi::freenow::MakeTokenFromJson(result);
  TEST(!token.m_token.empty(), ());
}

UNIT_TEST(Freenow_MakeTokenFromJson)
{
  auto const token = taxi::freenow::MakeTokenFromJson(kTokenResponse);
  TEST(!token.m_token.empty(), ());
  TEST_NOT_EQUAL(token.m_expiredTime.time_since_epoch().count(), 0, ());
}

UNIT_TEST(Freenow_MakeProductsFromJson)
{
  auto const products = taxi::freenow::MakeProductsFromJson(kServiceTypesResponse);
  TEST_EQUAL(products.size(), 1, ());
  TEST_EQUAL(products.back().m_name, "Taxi", ());
  TEST_EQUAL(products.back().m_time, "0", ());
  TEST_EQUAL(products.back().m_price, "5000GBP", ());
  TEST_EQUAL(products.back().m_currency, "GBP", ());
}

UNIT_CLASS_TEST(Runner, Freenow_GetAvailableProducts)
{
  taxi::freenow::Api api("http://localhost:34568/partners");
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
