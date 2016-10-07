#include "testing/testing.hpp"

#include "partners_api/uber_api.hpp"

#include "geometry/latlon.hpp"

#include "3party/jansson/myjansson.hpp"

UNIT_TEST(Uber_GetProducts)
{
  ms::LatLon pos(38.897724, -77.036531);

  TEST(!uber::RawApi::GetProducts(pos).empty(), ());
}

UNIT_TEST(Uber_GetTimes)
{
  ms::LatLon pos(38.897724, -77.036531);

  my::Json timeRoot(uber::RawApi::GetEstimatedTime(pos).c_str());
  auto const timesArray = json_object_get(timeRoot.get(), "times");

  TEST(json_is_array(timesArray), ());
  TEST_GREATER(json_array_size(timesArray), 0, ());

  auto const timeSize = json_array_size(timesArray);
  for (size_t i = 0; i < timeSize; ++i)
  {
    string name;
    json_int_t estimatedTime = 0;
    auto const item = json_array_get(timesArray, i);

    try
    {
      my::FromJSONObject(item, "display_name", name);
      my::FromJSONObject(item, "estimate", estimatedTime);
    }
    catch (my::Json::Exception const & e)
    {
      TEST(false, (e.Msg()));
    }

    string estimated = strings::to_string(estimatedTime);

    TEST(!name.empty(), ());
    TEST(!estimated.empty(), ());
  }
}

UNIT_TEST(Uber_GetPrices)
{
  ms::LatLon from(38.897724, -77.036531);
  ms::LatLon to(38.862416, -76.883316);

  my::Json priceRoot(uber::RawApi::GetEstimatedPrice(from, to).c_str());
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
      my::FromJSONObject(item, "product_id", productId);
      my::FromJSONObject(item, "estimate", price);

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
  ms::LatLon from(38.897724, -77.036531);
  ms::LatLon to(38.862416, -76.883316);

  uber::ProductMaker maker;

  maker.Reset(reqId);
  maker.SetTimes(reqId, uber::RawApi::GetEstimatedTime(from));
  maker.SetPrices(reqId, uber::RawApi::GetEstimatedPrice(from, to));
  maker.MakeProducts(reqId, [reqId](vector<uber::Product> const & products, size_t const requestId)
  {
    TEST(!products.empty(), ());
    TEST_EQUAL(requestId, reqId, ());

    for (auto const & product : products)
    {
      TEST(!product.m_productId.empty() &&
           !product.m_name.empty() &&
           !product.m_time.empty() &&
           !product.m_price.empty(),());
    }
  });

  ++reqId;

  maker.Reset(reqId);
  maker.SetTimes(reqId, uber::RawApi::GetEstimatedTime(from));
  maker.SetPrices(reqId, uber::RawApi::GetEstimatedPrice(from, to));

  maker.MakeProducts(reqId + 1, [reqId](vector<uber::Product> const & products, size_t const requestId)
  {
    TEST(false, ());
  });
}
