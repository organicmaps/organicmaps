#include "uber_api.hpp"

#include "platform/http_client.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/future.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

#define UBER_SERVER_TOKEN ""
#define UBER_CLIENT_ID ""

using namespace platform;

namespace
{
string RunSimpleHttpRequest(string const & url)
{
  HttpClient request(url);

  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    return request.ServerResponse();
  }

  return {};
}

bool CheckUberAnswer(json_t const * answer)
{
  // Uber products are not available at this point.
  if (!json_is_array(answer))
    return false;

  if (json_array_size(answer) <= 0)
    return false;

  return true;
}

void FillProducts(json_t const * time, json_t const * price, vector<uber::Product> & products)
{
  auto const timeSize = json_array_size(time);
  for (size_t i = 0; i < timeSize; ++i)
  {
    uber::Product product;
    json_int_t estimatedTime = 0;
    auto const item = json_array_get(time, i);
    my::FromJSONObject(item, "display_name", product.m_name);
    my::FromJSONObject(item, "estimate", estimatedTime);
    product.m_time = strings::to_string(estimatedTime);
    products.push_back(product);
  }

  auto const priceSize = json_array_size(price);
  for (size_t i = 0; i < priceSize; ++i)
  {
    string name;
    auto const item = json_array_get(price, i);

    my::FromJSONObject(item, "display_name", name);
    auto const it = find_if(products.begin(), products.end(), [&name](uber::Product const & product){
      return product.m_name == name;
    });
    if (it == products.end())
      continue;

    my::FromJSONObject(item, "product_id", it->m_productId);
    my::FromJSONObject(item, "estimate", it->m_price);
  }

  products.erase(remove_if(products.begin(), products.end(), [](uber::Product const & p){
                   return p.m_name.empty() || p.m_productId.empty() || p.m_time.empty() ||
                          p.m_price.empty();
                 }), products.end());
}

void GetAvailableProductsAsync(ms::LatLon const & from, ms::LatLon const & to, size_t const requestId,
                              uber::ProductsCallback const & fn)
{
  auto time = async(launch::async, uber::RawApi::GetEstimatedTime, ref(from));
  auto price = async(launch::async, uber::RawApi::GetEstimatedPrice, ref(from), ref(to));

  vector<uber::Product> products;

  try
  {
    my::Json timeRoot(time.get().c_str());
    my::Json priceRoot(price.get().c_str());
    auto const timesArray = json_object_get(timeRoot.get(), "times");
    auto const pricesArray = json_object_get(priceRoot.get(), "prices");
    if (CheckUberAnswer(timesArray) && CheckUberAnswer(pricesArray))
    {
      FillProducts(timesArray, pricesArray, products);
    }
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
    products.clear();
  }

  fn(products, requestId);
}
}  // namespace

namespace uber
{
// static
string RawApi::GetProducts(ms::LatLon const & pos)
{
  stringstream url;
  url << "https://api.uber.com/v1/products?server_token=" << UBER_SERVER_TOKEN <<
         "&latitude=" << static_cast<float>(pos.lat) <<
         "&longitude=" << static_cast<float>(pos.lon);

  return RunSimpleHttpRequest(url.str());
}

// static
string RawApi::GetEstimatedTime(ms::LatLon const & pos)
{
//  stringstream url;
//  url << "https://api.uber.com/v1/products?server_token=" << UBER_SERVER_TOKEN <<
//         "&start_latitude=" << static_cast<float>(pos.lat) <<
//         "&start_longitude=" << static_cast<float>(pos.lon);

//  return RunSimpleHttpRequest(url.str());
  return R"({
         "times":[
            {
               "localized_display_name":"uberPOOL",
               "estimate":180,
               "display_name":"uberPOOL",
               "product_id":"26546650-e557-4a7b-86e7-6a3942445247"
            },
            {
               "localized_display_name":"uberX",
               "estimate":180,
               "display_name":"uberX",
               "product_id":"a1111c8c-c720-46c3-8534-2fcdd730040d"
            },
            {
               "localized_display_name":"uberXL",
               "estimate":420,
               "display_name":"uberXL",
               "product_id":"821415d8-3bd5-4e27-9604-194e4359a449"
            },
            {
               "localized_display_name":"UberBLACK",
               "estimate":180,
               "display_name":"UberBLACK",
               "product_id":"d4abaae7-f4d6-4152-91cc-77523e8165a4"
            }
         ]
      })";
}

// static
string RawApi::GetEstimatedPrice(ms::LatLon const & from, ms::LatLon const & to)
{
//  stringstream url;
//  url << "https://api.uber.com/v1/products?server_token=" << UBER_SERVER_TOKEN <<
//         "&start_latitude=" << static_cast<float>(from.lat) <<
//         "&start_longitude=" << static_cast<float>(from.lon) <<
//         "&end_latitude=" << static_cast<float>(to.lat) <<
//         "&end_longitude=" << static_cast<float>(to.lon);

//  return RunSimpleHttpRequest(url.str());
  return R"({
         "prices":[
           {
             "product_id": "26546650-e557-4a7b-86e7-6a3942445247",
             "currency_code": "USD",
             "display_name": "POOL",
             "estimate": "$7",
             "low_estimate": 7,
             "high_estimate": 7,
             "surge_multiplier": 1,
             "duration": 640,
             "distance": 5.34
           },
           {
             "product_id": "08f17084-23fd-4103-aa3e-9b660223934b",
             "currency_code": "USD",
             "display_name": "UberBLACK",
             "estimate": "$23-29",
             "low_estimate": 23,
             "high_estimate": 29,
             "surge_multiplier": 1,
             "duration": 640,
             "distance": 5.34
           },
           {
             "product_id": "9af0174c-8939-4ef6-8e91-1a43a0e7c6f6",
             "currency_code": "USD",
             "display_name": "UberSUV",
             "estimate": "$36-44",
             "low_estimate": 36,
             "high_estimate": 44,
             "surge_multiplier": 1.25,
             "duration": 640,
             "distance": 5.34
           },
           {
             "product_id": "aca52cea-9701-4903-9f34-9a2395253acb",
             "currency_code": null,
             "display_name": "uberTAXI",
             "estimate": "Metered",
             "low_estimate": null,
             "high_estimate": null,
             "surge_multiplier": 1,
             "duration": 640,
             "distance": 5.34
           },
           {
             "product_id": "a27a867a-35f4-4253-8d04-61ae80a40df5",
             "currency_code": "USD",
             "display_name": "uberX",
             "estimate": "$15",
             "low_estimate": 15,
             "high_estimate": 15,
             "surge_multiplier": 1,
             "duration": 640,
             "distance": 5.34
           }
         ]
       })";
}

size_t Api::GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                                ProductsCallback const & fn)
{
  lock_guard<mutex> lock(m_mutex);

  m_thread.reset();
  static size_t requestId = 0;
  ++requestId;
  m_thread = unique_ptr<threads::SimpleThread, threadDeleter>(
        new threads::SimpleThread(GetAvailableProductsAsync, ref(from), ref(to), requestId, ref(fn)),
        [](threads::SimpleThread * ptr) { ptr->join(); delete ptr; });

  return requestId;
}

// static
string Api::GetRideRequestLink(string const & m_productId, ms::LatLon const & from,
                               ms::LatLon const & to)
{
  stringstream url;
  url << "uber://?client_id=" << UBER_CLIENT_ID <<
        "&action=setPickup&product_id=" << m_productId <<
        "&pickup[latitude]=" << static_cast<float>(from.lat) <<
        "&pickup[longitude]=" << static_cast<float>(from.lon) <<
        "&dropoff[latitude]=" << static_cast<float>(to.lat)<<
        "&dropoff[longitude]=" << static_cast<float>(to.lon);

  return url.str();
}
}  // namespace uber
