#include "partners_api/uber_api.hpp"
#include "partners_api/utils.hpp"

#include "platform/platform.hpp"

#include "geometry/latlon.hpp"

#include "base/logging.hpp"
#include "base/thread.hpp"

#include "std/iomanip.hpp"
#include "std/sstream.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

using namespace platform;

namespace
{
bool RunSimpleHttpRequest(std::string const & url, std::string & result)
{
  platform::HttpClient request(url);
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    result = request.ServerResponse();
    return true;
  }
  return false;
}

bool CheckUberResponse(json_t const * answer)
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

bool IsIncomplete(taxi::Product const & p)
{
  return p.m_name.empty() || p.m_productId.empty() || p.m_time.empty() || p.m_price.empty();
}

void FillProducts(json_t const * time, json_t const * price, vector<taxi::Product> & products)
{
  // Fill data from time.
  auto const timeSize = json_array_size(time);
  for (size_t i = 0; i < timeSize; ++i)
  {
    taxi::Product product;
    int64_t estimatedTime = 0;
    auto const item = json_array_get(time, i);
    FromJSONObject(item, "display_name", product.m_name);
    FromJSONObject(item, "estimate", estimatedTime);
    product.m_time = strings::to_string(estimatedTime);
    products.push_back(move(product));
  }

  // Fill data from price.
  auto const priceSize = json_array_size(price);
  for (size_t i = 0; i < priceSize; ++i)
  {
    string name;
    auto const item = json_array_get(price, i);

    FromJSONObject(item, "display_name", name);
    auto const it = find_if(products.begin(), products.end(), [&name](taxi::Product const & product)
    {
      return product.m_name == name;
    });

    if (it == products.end())
      continue;

    FromJSONObject(item, "product_id", it->m_productId);
    FromJSONObject(item, "estimate", it->m_price);

    // The field currency_code can contain null in case when price equal to Metered.
    auto const currency = json_object_get(item, "currency_code");
    if (currency != nullptr && !json_is_null(currency))
      it->m_currency = json_string_value(currency);
  }

  products.erase(remove_if(products.begin(), products.end(), IsIncomplete), products.end());
}

void MakeFromJson(char const * times, char const * prices, vector<taxi::Product> & products)
{
  products.clear();
  try
  {
    my::Json timesRoot(times);
    my::Json pricesRoot(prices);
    auto const timesArray = json_object_get(timesRoot.get(), "times");
    auto const pricesArray = json_object_get(pricesRoot.get(), "prices");
    if (CheckUberResponse(timesArray) && CheckUberResponse(pricesArray))
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

namespace taxi
{
namespace uber
{
string const kEstimatesUrl = "https://api.uber.com/v1/estimates";
string const kProductsUrl = "https://api.uber.com/v1/products";

// static
bool RawApi::GetProducts(ms::LatLon const & pos, string & result,
                         std::string const & baseUrl /* = kProductsUrl */)
{
  ostringstream url;
  url << fixed << setprecision(6) << baseUrl << "?server_token=" << UBER_SERVER_TOKEN
      << "&latitude=" << pos.lat << "&longitude=" << pos.lon;

  return RunSimpleHttpRequest(url.str(), result);
}

// static
bool RawApi::GetEstimatedTime(ms::LatLon const & pos, string & result,
                              std::string const & baseUrl /* = kEstimatesUrl */)
{
  ostringstream url;
  url << fixed << setprecision(6) << baseUrl << "/time?server_token=" << UBER_SERVER_TOKEN
      << "&start_latitude=" << pos.lat << "&start_longitude=" << pos.lon;

  return RunSimpleHttpRequest(url.str(), result);
}

// static
bool RawApi::GetEstimatedPrice(ms::LatLon const & from, ms::LatLon const & to, string & result,
                               std::string const & baseUrl /* = kEstimatesUrl */)
{
  ostringstream url;
  url << fixed << setprecision(6) << baseUrl << "/price?server_token=" << UBER_SERVER_TOKEN
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

void ProductMaker::SetError(uint64_t const requestId, taxi::ErrorCode code)
{
  lock_guard<mutex> lock(m_mutex);

  if (requestId != m_requestId)
    return;

  m_error = make_unique<taxi::ErrorCode>(code);
}

void ProductMaker::MakeProducts(uint64_t const requestId, ProductsCallback const & successFn,
                                ErrorProviderCallback const & errorFn)
{
  ASSERT(successFn, ());
  ASSERT(errorFn, ());

  vector<Product> products;
  unique_ptr<taxi::ErrorCode> error;
  {
    lock_guard<mutex> lock(m_mutex);

    if (requestId != m_requestId || !m_times || !m_prices)
      return;

    if (!m_error)
    {
      if (!m_times->empty() && !m_prices->empty())
        MakeFromJson(m_times->c_str(), m_prices->c_str(), products);

      if (products.empty())
        m_error = my::make_unique<taxi::ErrorCode>(ErrorCode::NoProducts);
    }

    if (m_error)
      error = my::make_unique<taxi::ErrorCode>(*m_error);

    // Reset m_times and m_prices because we need to call callback only once.
    m_times.reset();
    m_prices.reset();
  }

  if (error)
    errorFn(*error);
  else
    successFn(products);
}

void Api::GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                               ProductsCallback const & successFn,
                               ErrorProviderCallback const & errorFn)
{
  ASSERT(successFn, ());
  ASSERT(errorFn, ());

  if (!IsDistanceSupported(from, to))
  {
    // TODO(a): Add ErrorCode::FarDistance and provide this error code.
    errorFn(ErrorCode::NoProducts);
    return;
  }

  auto const reqId = ++m_requestId;
  auto const maker = m_maker;
  auto const baseUrl = m_baseUrl;

  maker->Reset(reqId);

  GetPlatform().RunTask(Platform::Thread::Network, [maker, from, reqId, baseUrl, successFn, errorFn]()
  {
    string result;
    if (!RawApi::GetEstimatedTime(from, result, baseUrl))
      maker->SetError(reqId, ErrorCode::RemoteError);

    maker->SetTimes(reqId, result);
    maker->MakeProducts(reqId, successFn, errorFn);
  });

  GetPlatform().RunTask(Platform::Thread::Network, [maker, from, to, reqId, baseUrl, successFn, errorFn]()
  {
    string result;
    if (!RawApi::GetEstimatedPrice(from, to, result, baseUrl))
      maker->SetError(reqId, ErrorCode::RemoteError);

    maker->SetPrices(reqId, result);
    maker->MakeProducts(reqId, successFn, errorFn);
  });
}

RideRequestLinks Api::GetRideRequestLinks(string const & productId, ms::LatLon const & from,
                                          ms::LatLon const & to) const
{
  stringstream url;
  url << fixed << setprecision(6)
      << "?client_id=" << UBER_CLIENT_ID << "&action=setPickup&product_id=" << productId
      << "&pickup[latitude]=" << from.lat << "&pickup[longitude]=" << from.lon
      << "&dropoff[latitude]=" << to.lat << "&dropoff[longitude]=" << to.lon;

  return {"uber://" + url.str(), "https://m.uber.com/ul" + url.str()};
}
}  // namespace uber
}  // namespace taxi
