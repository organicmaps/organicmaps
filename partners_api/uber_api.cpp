#include "uber_api.hpp"

#include "platform/http_client.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/chrono.hpp"
#include "std/future.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

using namespace platform;

namespace
{
uint32_t const kHttpMinWait = 10;

string RunSimpleHttpRequest(string const & url)
{
  HttpClient request(url);
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    return request.ServerResponse();
  }
  return {};
}

/// Feature should refers to a shared state.
template <typename T>
bool WaitForFeature(future<T> const & f, uint32_t waitMillisec, atomic<bool> const & runFlag)
{
  future_status status = future_status::deferred;
  while (runFlag && status != future_status::ready)
  {
    status = f.wait_for(milliseconds(waitMillisec));
  }

  return runFlag;
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
  // Fill data from time.
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

  // Fill data from price.
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

    // The field currency_code can contain null in case when price equal to Metered.
    auto const currency = json_object_get(item, "currency_code");
    if (currency != nullptr && !json_is_null(currency))
      it->m_currency = json_string_value(currency);
  }

  products.erase(remove_if(products.begin(), products.end(), [](uber::Product const & p){
                   return p.m_name.empty() || p.m_productId.empty() || p.m_time.empty() ||
                          p.m_price.empty();
                 }), products.end());
}

void GetAvailableProductsAsync(ms::LatLon const from, ms::LatLon const to,
                               size_t const requestId, atomic<bool> const & runFlag,
                               uber::ProductsCallback const fn)
{
  auto time = async(launch::async, uber::RawApi::GetEstimatedTime, ref(from));
  auto price = async(launch::async, uber::RawApi::GetEstimatedPrice, ref(from), ref(to));

  vector<uber::Product> products;

  if (!WaitForFeature(time, kHttpMinWait, runFlag) || !WaitForFeature(price, kHttpMinWait, runFlag))
  {
    return;
  }

  try
  {
    string timeStr = time.get();
    string priceStr = price.get();

    if (timeStr.empty() || priceStr.empty())
    {
      LOG(LWARNING, ("Time or price is empty, time:", timeStr, "; price:", priceStr));
      return;
    }

    my::Json timeRoot(timeStr.c_str());
    my::Json priceRoot(priceStr.c_str());
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
  url << "https://api.uber.com/v1/products?server_token=" << UBER_SERVER_TOKEN
      << "&latitude=" << pos.lat << "&longitude=" << pos.lon;

  return RunSimpleHttpRequest(url.str());
}

// static
string RawApi::GetEstimatedTime(ms::LatLon const & pos)
{
  stringstream url;
  url << "https://api.uber.com/v1/estimates/time?server_token=" << UBER_SERVER_TOKEN
      << "&start_latitude=" << pos.lat << "&start_longitude=" << pos.lon;

  return RunSimpleHttpRequest(url.str());
}

// static
string RawApi::GetEstimatedPrice(ms::LatLon const & from, ms::LatLon const & to)
{
  stringstream url;
  url << "https://api.uber.com/v1/estimates/price?server_token=" << UBER_SERVER_TOKEN
      << "&start_latitude=" << from.lat << "&start_longitude=" << from.lon
      << "&end_latitude=" << to.lat << "&end_longitude=" << to.lon;

  return RunSimpleHttpRequest(url.str());
}

Api::~Api()
{
  ResetThread();
}

size_t Api::GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                                ProductsCallback const & fn)
{
  static size_t requestId = 0;
  ResetThread();
  m_runFlag = true;
  m_thread = make_unique<threads::SimpleThread>(GetAvailableProductsAsync, from, to,
                                                ++requestId, ref(m_runFlag), fn);

  return requestId;
}

// static
string Api::GetRideRequestLink(string const & productId, ms::LatLon const & from,
                               ms::LatLon const & to)
{
  stringstream url;
  url << "uber://?client_id=" << UBER_CLIENT_ID << "&action=setPickup&product_id=" << productId
      << "&pickup[latitude]=" << static_cast<float>(from.lat)
      << "&pickup[longitude]=" << static_cast<float>(from.lon)
      << "&dropoff[latitude]=" << static_cast<float>(to.lat)
      << "&dropoff[longitude]=" << static_cast<float>(to.lon);

  return url.str();
}

void Api::ResetThread()
{
  m_runFlag = false;

  if (m_thread)
  {
    m_thread->join();
    m_thread.reset();
  }
}
}  // namespace uber
