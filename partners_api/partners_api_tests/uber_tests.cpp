#include "testing/testing.hpp"

#include "partners_api/uber_api.hpp"

#include "geometry/latlon.hpp"

#include "platform/platform.hpp"

#include "std/algorithm.hpp"
#include "std/atomic.hpp"
#include "std/mutex.hpp"

#include "3party/jansson/myjansson.hpp"

using namespace taxi;

namespace
{
using Runner = Platform::ThreadRunner;

bool IsComplete(Product const & product)
{
  return !product.m_productId.empty() && !product.m_name.empty() && !product.m_time.empty() &&
         !product.m_price.empty();
}
}  // namespace

UNIT_TEST(Uber_GetProducts)
{
  ms::LatLon const pos(38.897724, -77.036531);
  string result;
  TEST(uber::RawApi::GetProducts(pos, result), ());
  TEST(!result.empty(), ());
}

UNIT_TEST(Uber_GetTimes)
{
  ms::LatLon const pos(38.897724, -77.036531);
  string result;

  TEST(uber::RawApi::GetEstimatedTime(pos, result), ());
  TEST(!result.empty(), ());

  my::Json timeRoot(result.c_str());
  auto const timesArray = json_object_get(timeRoot.get(), "times");

  TEST(json_is_array(timesArray), ());
  TEST_GREATER(json_array_size(timesArray), 0, ());

  auto const timeSize = json_array_size(timesArray);
  for (size_t i = 0; i < timeSize; ++i)
  {
    string name;
    int64_t estimatedTime = 0;
    auto const item = json_array_get(timesArray, i);

    try
    {
      FromJSONObject(item, "display_name", name);
      FromJSONObject(item, "estimate", estimatedTime);
    }
    catch (my::Json::Exception const & e)
    {
      TEST(false, (e.Msg()));
    }

    string const estimated = strings::to_string(estimatedTime);

    TEST(!name.empty(), ());
    TEST(!estimated.empty(), ());
  }
}

UNIT_TEST(Uber_GetPrices)
{
  ms::LatLon const from(38.897724, -77.036531);
  ms::LatLon const to(38.862416, -76.883316);
  string result;

  TEST(uber::RawApi::GetEstimatedPrice(from, to, result), ());
  TEST(!result.empty(), ());

  my::Json priceRoot(result.c_str());
  auto const pricesArray = json_object_get(priceRoot.get(), "prices");

  TEST(json_is_array(pricesArray), ());
  TEST_GREATER(json_array_size(pricesArray), 0, ());

  auto const pricesSize = json_array_size(pricesArray);
  for (size_t i = 0; i < pricesSize; ++i)
  {
    string productId;
    string price;
    string currency;

    auto const item = json_array_get(pricesArray, i);

    try
    {
      FromJSONObject(item, "product_id", productId);
      FromJSONObject(item, "estimate", price);

      auto const val = json_object_get(item, "currency_code");
      if (val != nullptr)
      {
        if (!json_is_null(val))
          currency = json_string_value(val);
        else
          currency = "null";
      }
    }
    catch (my::Json::Exception const & e)
    {
      TEST(false, (e.Msg()));
    }

    TEST(!productId.empty(), ());
    TEST(!price.empty(), ());
    TEST(!currency.empty(), ());
  }
}

UNIT_TEST(Uber_ProductMaker)
{
  size_t reqId = 1;
  ms::LatLon const from(38.897724, -77.036531);
  ms::LatLon const to(38.862416, -76.883316);

  vector<Product> returnedProducts;

  uber::ProductMaker maker;

  string times;
  string prices;

  auto const errorCallback = [](ErrorCode const code) { TEST(false, ()); };

  TEST(uber::RawApi::GetEstimatedTime(from, times), ());
  TEST(uber::RawApi::GetEstimatedPrice(from, to, prices), ());

  maker.Reset(reqId);
  maker.SetTimes(reqId, times);
  maker.SetPrices(reqId, prices);
  maker.MakeProducts(reqId,
                     [&returnedProducts](vector<Product> const & products) {
                       returnedProducts = products;
                     },
                     errorCallback);

  TEST(!returnedProducts.empty(), ());

  for (auto const & product : returnedProducts)
    TEST(IsComplete(product), ());

  ++reqId;

  TEST(uber::RawApi::GetEstimatedTime(from, times), ());
  TEST(uber::RawApi::GetEstimatedPrice(from, to, prices), ());

  maker.Reset(reqId);
  maker.SetTimes(reqId, times);
  maker.SetPrices(reqId, prices);

  maker.MakeProducts(reqId + 1, [](vector<Product> const & products)
  {
    TEST(false, ());
  }, errorCallback);
}

UNIT_CLASS_TEST(Runner, Uber_GetAvailableProducts)
{
  taxi::uber::Api api("http://localhost:34568/partners");
  ms::LatLon const from(55.796918, 37.537859);
  ms::LatLon const to(55.758213, 37.616093);

  std::vector<taxi::Product> resultProducts;
  std::atomic<int> counter(0);

  api.GetAvailableProducts(from, to,
                           [&resultProducts, &counter](std::vector<taxi::Product> const & products) {
                             resultProducts = products;
                             ++counter;
                             testing::Notify();
                           },
                           [](taxi::ErrorCode const code) {
                             TEST(false, (code));
                             testing::Notify();
                           });

  testing::Wait();

  TEST(!resultProducts.empty(), ());
  TEST_EQUAL(counter, 1, ());

  counter = 0;
  taxi::ErrorCode errorCode = taxi::ErrorCode::RemoteError;
  ms::LatLon const farPos(56.838197, 35.908507);
  api.GetAvailableProducts(from, farPos,
                           [](std::vector<taxi::Product> const & products) {
                             TEST(false, ());
                             testing::Notify();
                           },
                           [&errorCode, &counter](taxi::ErrorCode const code) {
                             errorCode = code;
                             ++counter;
                             testing::Notify();
                           });

  testing::Wait();

  TEST_EQUAL(errorCode, taxi::ErrorCode::NoProducts, ());
  TEST_EQUAL(counter, 1, ());
}

UNIT_TEST(Uber_GetRideRequestLinks)
{
  taxi::uber::Api api;
  ms::LatLon const from(55.796918, 37.537859);
  ms::LatLon const to(55.758213, 37.616093);

  auto const links = api.GetRideRequestLinks("" /* productId */, from, to);

  TEST(!links.m_deepLink.empty(), ());
  TEST(!links.m_universalLink.empty(), ());
}
