#include "partners_api/yandex_api.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "geometry/latlon.hpp"

#include "base/logging.hpp"
#include "base/thread.hpp"

#include "std/target_os.hpp"

#include <iomanip>
#include <limits>
#include <sstream>

#include "3party/jansson/myjansson.hpp"

#include "private.h"

namespace
{
bool RunSimpleHttpRequest(std::string const & url, std::string & result)
{
  platform::HttpClient request(url);
  request.SetRawHeader("Accept", "application/json");
  request.SetRawHeader("YaTaxi-Api-Key", YANDEX_API_KEY);
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    result = request.ServerResponse();
    return true;
  }

  return false;
}

bool CheckYandexResponse(json_t const * answer)
{
  if (answer == nullptr)
    return false;

  if (!json_is_array(answer))
    return false;

  if (json_array_size(answer) <= 0)
    return false;

  return true;
}
}  // namespace

namespace taxi
{
namespace yandex
{
std::string const kTaxiInfoUrl = "https://taxi-routeinfo.taxi.yandex.net";

Countries const kEnabledCountries = {{{}}};

bool RawApi::GetTaxiInfo(ms::LatLon const & from, ms::LatLon const & to, std::string & result,
                         std::string const & baseUrl /* = kTaxiInfoUrl */)
{
  std::ostringstream url;
  url << std::fixed << std::setprecision(6) << baseUrl << "/taxi_info?clid=" << YANDEX_CLIENT_ID
      << "&rll=" << from.lon << "," << from.lat << "~" << to.lon << "," << to.lat
      << "&class=econom";

  return RunSimpleHttpRequest(url.str(), result);
}

/// Requests list of available products from Yandex.
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

  auto const baseUrl = m_baseUrl;

  GetPlatform().RunTask(Platform::Thread::Network, [from, to, baseUrl, successFn, errorFn]()
  {
    std::string result;
    if (!RawApi::GetTaxiInfo(from, to, result, baseUrl))
    {
      errorFn(ErrorCode::RemoteError);
      return;
    }

    std::vector<Product> products;
    try
    {
      MakeFromJson(result, products);
    }
    catch (my::Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg()));
      products.clear();
    }

    if (products.empty())
      errorFn(ErrorCode::NoProducts);
    else
      successFn(products);

  });
}

/// Returns link which allows you to launch the Yandex app.
RideRequestLinks Api::GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                          ms::LatLon const & to) const
{
  std::ostringstream deepLink;

#if defined(OMIM_OS_IPHONE)
  deepLink << "https://3.redirect.appmetrica.yandex.com/route?start-lat=" << from.lat
           << "&start-lon=" << from.lon << "&end-lat=" << to.lat << "&end-lon=" << to.lon
           << "&utm_source=mapsme&appmetrica_tracking_id=" << YANDEX_TRACKING_ID;
#elif defined(OMIM_OS_ANDROID)
  deepLink << "https://redirect.appmetrica.yandex.com/serve/" << YANDEX_TRACKING_ID << "?startlat="
           << from.lat << "&startlon=" << from.lon << "&endlat=" << to.lat << "&endlon=" << to.lon;
#endif

  return {deepLink.str(), deepLink.str()};
}

void MakeFromJson(std::string const & src, std::vector<taxi::Product> & products)
{
  products.clear();

  my::Json root(src.c_str());
  auto const productsArray = json_object_get(root.get(), "options");
  if (!CheckYandexResponse(productsArray))
    return;

  std::string currency;
  FromJSONObject(root.get(), "currency", currency);

  auto const productsSize = json_array_size(productsArray);
  for (size_t i = 0; i < productsSize; ++i)
  {
    taxi::Product product;
    double time = 0.0;
    double price = 0.0;
    auto const item = json_array_get(productsArray, i);

    FromJSONObjectOptionalField(item, "waiting_time", time);
    // Temporary unavailable.
    if (time == 0.0)
      continue;

    FromJSONObject(item, "class_name", product.m_name);
    FromJSONObject(item, "price", price);

    product.m_price = strings::to_string(price);
    product.m_time = strings::to_string(static_cast<int64_t>(time));
    product.m_currency = currency;
    products.push_back(move(product));
  }
}
}  // namespace yandex
}  // namespace taxi
