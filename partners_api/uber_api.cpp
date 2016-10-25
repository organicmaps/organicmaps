#include "partners_api/uber_api.hpp"

#include "platform/http_client.hpp"

#include "geometry/latlon.hpp"

#include "base/logging.hpp"
#include "base/thread.hpp"

#include "std/iomanip.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

using namespace platform;

namespace
{
bool RunSimpleHttpRequest(string const & url, string & result)
{
  HttpClient request(url);
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    result = request.ServerResponse();
    return true;
  }
  return false;
}

bool CheckUberAnswer(json_t const * answer)
{
  if (answer == nullptr)
    return false;

  // Uber products are not available at this point.
  if (!json_is_array(answer))
    return false;

  if (json_array_size(answer) <= 0)
    return false;

  return true;
}

bool IsIncomplete(uber::Product const & p)
{
  return p.m_name.empty() || p.m_productId.empty() || p.m_time.empty() || p.m_price.empty();
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
    products.push_back(move(product));
  }

  // Fill data from price.
  auto const priceSize = json_array_size(price);
  for (size_t i = 0; i < priceSize; ++i)
  {
    string name;
    auto const item = json_array_get(price, i);

    my::FromJSONObject(item, "display_name", name);
    auto const it = find_if(products.begin(), products.end(), [&name](uber::Product const & product)
    {
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

  products.erase(remove_if(products.begin(), products.end(), IsIncomplete), products.end());
}

void MakeFromJson(char const * times, char const * prices, vector<uber::Product> & products)
{
  products.clear();
  try
  {
    my::Json timesRoot(times);
    my::Json pricesRoot(prices);
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
}
}  // namespace

namespace uber
{
// static
bool RawApi::GetProducts(ms::LatLon const & pos, string & result)
{
  stringstream url;
  url << fixed << setprecision(6)
      << "https://api.uber.com/v1/products?server_token=" << UBER_SERVER_TOKEN
      << "&latitude=" << pos.lat << "&longitude=" << pos.lon;

  return RunSimpleHttpRequest(url.str(), result);
}

// static
bool RawApi::GetEstimatedTime(ms::LatLon const & pos, string & result)
{
  stringstream url;
  url << fixed << setprecision(6)
      << "https://api.uber.com/v1/estimates/time?server_token=" << UBER_SERVER_TOKEN
      << "&start_latitude=" << pos.lat << "&start_longitude=" << pos.lon;

  return RunSimpleHttpRequest(url.str(), result);
}

// static
bool RawApi::GetEstimatedPrice(ms::LatLon const & from, ms::LatLon const & to, string & result)
{
  stringstream url;
  url << fixed << setprecision(6)
      << "https://api.uber.com/v1/estimates/price?server_token=" << UBER_SERVER_TOKEN
      << "&start_latitude=" << from.lat << "&start_longitude=" << from.lon
      << "&end_latitude=" << to.lat << "&end_longitude=" << to.lon;

  return RunSimpleHttpRequest(url.str(), result);
}

void ProductMaker::Reset(uint64_t const requestId)
{
  lock_guard<mutex> lock(m_mutex);

  m_requestId = requestId;
  m_times.reset();
  m_prices.reset();
}

void ProductMaker::SetTimes(uint64_t const requestId, string const & times)
{
  lock_guard<mutex> lock(m_mutex);

  if (requestId != m_requestId)
    return;

  m_times = make_unique<string>(times);
}

void ProductMaker::SetPrices(uint64_t const requestId, string const & prices)
{
  lock_guard<mutex> lock(m_mutex);

  if (requestId != m_requestId)
    return;

  m_prices = make_unique<string>(prices);
}

void ProductMaker::MakeProducts(uint64_t const requestId, ProductsCallback const & successFn,
                                ErrorCallback const & errorFn)
{
  vector<uber::Product> products;
  {
    lock_guard<mutex> lock(m_mutex);

    if (requestId != m_requestId || !m_times || !m_prices)
      return;

    if (!m_times->empty() && !m_prices->empty())
      MakeFromJson(m_times->c_str(), m_prices->c_str(), products);
    else
      LOG(LWARNING, ("Time or price is empty, time:", *m_times, "; price:", *m_prices));
  }

  if (products.empty())
    errorFn(ErrorCode::NoProducts, requestId);
  else
    successFn(products, requestId);
}

uint64_t Api::GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                                   ProductsCallback const & successFn, ErrorCallback const & errorFn)
{
  auto const reqId = ++m_requestId;
  auto const maker = m_maker;

  maker->Reset(reqId);

  threads::SimpleThread([maker, from, reqId, successFn, errorFn]()
  {
    string result;
    if (!RawApi::GetEstimatedTime(from, result))
    {
      errorFn(ErrorCode::RemoteError, reqId);
      return;
    }

    maker->SetTimes(reqId, result);
    maker->MakeProducts(reqId, successFn, errorFn);
  }).detach();

  threads::SimpleThread([maker, from, to, reqId, successFn, errorFn]()
  {
    string result;
    if (!RawApi::GetEstimatedPrice(from, to, result))
    {
      errorFn(ErrorCode::RemoteError, reqId);
      return;
    }

    maker->SetPrices(reqId, result);
    maker->MakeProducts(reqId, successFn, errorFn);
  }).detach();

  return reqId;
}

// static
RideRequestLinks Api::GetRideRequestLinks(string const & productId, ms::LatLon const & from,
                                          ms::LatLon const & to)
{
  stringstream url;
  url << fixed << setprecision(6)
      << "?client_id=" << UBER_CLIENT_ID << "&action=setPickup&product_id=" << productId
      << "&pickup[latitude]=" << from.lat << "&pickup[longitude]=" << from.lon
      << "&dropoff[latitude]=" << to.lat << "&dropoff[longitude]=" << to.lon;

  return {"uber://" + url.str(), "https://m.uber.com/ul" + url.str()};
}
}  // namespace uber
