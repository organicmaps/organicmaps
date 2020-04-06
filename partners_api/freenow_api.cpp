#include "partners_api/freenow_api.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/url.hpp"

#include "geometry/latlon.hpp"

#include "base/logging.hpp"

#include "std/target_os.hpp"

#include <iomanip>
#include <limits>
#include <sstream>

#include "3party/jansson/myjansson.hpp"

#include "private.h"

namespace
{
auto const kTimeoutSec = 15;
}  // namespace

namespace taxi
{
namespace freenow
{
std::string const kTaxiEndpoint = "https://api.live.free-now.com/publicapigatewayservice/v1";

bool RawApi::GetAccessToken(std::string & result, std::string const & baseUrl /* = kTaxiEndpoint */)
{
  platform::HttpClient request(url::Join(baseUrl, "oauth/token"));
  request.SetTimeout(kTimeoutSec);
  request.SetBodyData("grant_type=client_credentials", "application/x-www-form-urlencoded");
  request.SetUserAndPassword(FREENOW_CLIENT_ID, FREENOW_CLIENT_SECRET);

  return request.RunHttpRequest(result);
}

bool RawApi::GetServiceTypes(ms::LatLon const & from, ms::LatLon const & to,
                             std::string const & token, std::string & result,
                             const std::string & baseUrl /* = kTaxiEndpoint */)
{
  std::ostringstream url;
  url << std::fixed << std::setprecision(6) << baseUrl << "/service-types?pickupLatitude="
      << from.m_lat << "&pickupLongitude=" << from.m_lon << "&destinationLatitude=" << to.m_lat
      << "&destinationLongitude=" << to.m_lon;

  platform::HttpClient request(url.str());
  request.SetTimeout(kTimeoutSec);
  request.SetRawHeader("Authorization", "Bearer " + token);
  request.SetRawHeader("Accept", "application/json");
  request.SetRawHeader("Accept-Language", languages::GetCurrentOrig());

  return request.RunHttpRequest(result);
}

void SafeToken::Set(Token const & token)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_token = token;
}

SafeToken::Token SafeToken::Get() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_token;
}

/// Requests list of available products from Freenow.
void Api::GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                               ProductsCallback const & successFn,
                               ErrorProviderCallback const & errorFn)
{
  ASSERT(successFn, ());
  ASSERT(errorFn, ());

  // TODO(a): Add ErrorCode::FarDistance and provide this error code.
  if (!IsDistanceSupported(from, to))
  {
    errorFn(ErrorCode::NoProducts);
    return;
  }

  static_assert(std::is_const<decltype(m_baseUrl)>::value, "");

  GetPlatform().RunTask(Platform::Thread::Network, [this, from, to, successFn, errorFn]()
  {
    auto token = m_accessToken.Get();
    if (token.m_expiredTime <= std::chrono::steady_clock::now() - std::chrono::seconds(kTimeoutSec))
    {
      std::string tokenSource;
      if (!RawApi::GetAccessToken(tokenSource, m_baseUrl))
      {
        errorFn(ErrorCode::RemoteError);
        return;
      }

      try
      {
        token = MakeTokenFromJson(tokenSource);
      }
      catch (base::Json::Exception const & e)
      {
        LOG(LERROR, (e.Msg(), tokenSource));
        errorFn(ErrorCode::NoProducts);
        return;
      }
      m_accessToken.Set(token);
    }

    std::string httpResult;
    if (!RawApi::GetServiceTypes(from, to, token.m_token, httpResult, m_baseUrl))
    {
      errorFn(ErrorCode::RemoteError);
      return;
    }

    std::vector<Product> products;
    try
    {
      products = MakeProductsFromJson(httpResult);
    }
    catch (base::Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg(), httpResult));
      products.clear();
    }

    if (products.empty())
      errorFn(ErrorCode::NoProducts);
    else
      successFn(products);
  });
}

/// Returns link which allows you to launch the Freenow app.
RideRequestLinks Api::GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                          ms::LatLon const & to) const
{
  std::string const universalLink = "https://mytaxi.onelink.me/HySP?pid=maps.me&c=in-app-referral-"
                                    "link_030320_my_pa_in_0_gl_gl_-_mx_mo_co_mx_af_-_ge_-_-_-_-_-"
                                    "&is_retargeting=true";
  std::ostringstream deepLink;
  deepLink << std::fixed << std::setprecision(6)
           << "mytaxi://de.mytaxi.passenger/order?pickup_coordinate=" << from.m_lat << ","
           << from.m_lon << "&destination_coordinate=" << to.m_lat << "," << to.m_lon
           << "&token=" << FREENOW_CLIENT_ID;
  url::Params const deepLinkParams = {{"af_dp", url::UrlEncode(deepLink.str())}};

  auto const result = url::Make(universalLink, deepLinkParams);
  return {result, result};
}

std::vector<taxi::Product> MakeProductsFromJson(std::string const & src)
{
  std::vector<taxi::Product> products;

  base::Json root(src.c_str());
  auto const serviceTypesArray = json_object_get(root.get(), "serviceTypes");

  auto const count = json_array_size(serviceTypesArray);
  for (size_t i = 0; i < count; ++i)
  {
    taxi::Product product;
    auto const item = json_array_get(serviceTypesArray, i);

    FromJSONObjectOptionalField(item, "id", product.m_productId);
    FromJSONObjectOptionalField(item, "displayName", product.m_name);

    auto const eta = json_object_get(item, "eta");
    if (json_is_object(eta))
    {
      uint64_t time = 0;
      FromJSONObjectOptionalField(eta, "value", time);
      product.m_time = strings::to_string(time);
    }

    auto const fare = json_object_get(item, "fare");
    if (json_is_object(fare))
    {
      FromJSONObjectOptionalField(fare, "displayValue", product.m_price);
      FromJSONObjectOptionalField(fare, "currencyCode", product.m_currency);
    }

    if (product.m_name.empty() || product.m_time.empty() || product.m_price.empty())
      continue;

    products.push_back(std::move(product));
  }

  return products;
}

SafeToken::Token MakeTokenFromJson(std::string const & src)
{
  SafeToken::Token result;
  base::Json root(src.c_str());
  FromJSONObject(root.get(), "access_token", result.m_token);
  uint64_t expiresInSeconds = 0;
  FromJSONObject(root.get(), "expires_in", expiresInSeconds);
  result.m_expiredTime = std::chrono::steady_clock::now() + std::chrono::seconds(expiresInSeconds);

  return result;
}
}  // namespace freenow
}  // namespace taxi
