#include "uber_api.hpp"

#include "platform/http_client.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/thread.hpp"

#include "std/chrono.hpp"
#include "std/future.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

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

void ProductMaker::Reset(size_t const requestId)
{
  lock_guard<mutex> lock(m_mutex);

  m_requestId = requestId;
  m_times.reset();
  m_prices.reset();
}

void ProductMaker::SetTimes(size_t const requestId, string const & times)
{
  lock_guard<mutex> lock(m_mutex);

  if (requestId != m_requestId)
    return;

  m_times = make_unique<string>(times);
}

void ProductMaker::SetPrices(size_t const requestId, string const & prices)
{
  lock_guard<mutex> lock(m_mutex);

  if (requestId != m_requestId)
    return;

  m_prices = make_unique<string>(prices);
}

void ProductMaker::MakeProducts(size_t const requestId, ProductsCallback const & fn)
{
  lock_guard<mutex> lock(m_mutex);

  if (requestId != m_requestId || !m_times || !m_prices)
    return;

  vector<uber::Product> products;

  if (m_times->empty() || m_prices->empty())
  {
    LOG(LWARNING, ("Time or price is empty, time:", *m_times, "; price:", *m_prices));
    fn(products, m_requestId);
    return;
  }

  try
  {
    my::Json timesRoot(m_times->c_str());
    my::Json pricesRoot(m_prices->c_str());
    auto const timesArray = json_object_get(timesRoot.get(), "times");
    auto const pricesArray = json_object_get(pricesRoot.get(), "prices");
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

size_t Api::GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                                ProductsCallback const & fn)
{
  static size_t requestId = 0;
  size_t reqId = ++requestId;

  m_maker->Reset(reqId);

  threads::SimpleThread([this, from, reqId, fn]()
  {
    m_maker->SetTimes(reqId, uber::RawApi::GetEstimatedTime(from));
    m_maker->MakeProducts(reqId, fn);
  }).detach();

  threads::SimpleThread([this, from, to, reqId, fn]()
  {
    m_maker->SetPrices(reqId, uber::RawApi::GetEstimatedPrice(from, to));
    m_maker->MakeProducts(reqId, fn);
  }).detach();

  return reqId;
}

// static
string Api::GetRideRequestLink(string const & productId, ms::LatLon const & from,
                               ms::LatLon const & to)
{
  stringstream url;
  url << "uber://?client_id=" << UBER_CLIENT_ID << "&action=setPickup&product_id=" << productId
      << "&pickup[latitude]=" << from.lat << "&pickup[longitude]=" << from.lon
      << "&dropoff[latitude]=" << to.lat << "&dropoff[longitude]=" << to.lon;

  return url.str();
}
}  // namespace uber
