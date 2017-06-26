#include "partners_api/yandex_api.hpp"

#include "platform/http_client.hpp"

#include "geometry/latlon.hpp"

#include "base/logging.hpp"
#include "base/thread.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

#include <iomanip>
#include <limits>
#include <sstream>

namespace
{
std::string const kTaxiInfoUrl = "https://taxi-routeinfo.taxi.yandex.net";
std::string g_yandexUrlForTesting = "";

bool RunSimpleHttpRequest(std::string const & url, std::string & result)
{
  platform::HttpClient request(url);
  request.SetRawHeader("Accept", "application/json");
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

std::string GetYandexURL()
{
  if (!g_yandexUrlForTesting.empty())
    return g_yandexUrlForTesting;

  return kTaxiInfoUrl;
}
}  // namespace

namespace taxi
{
namespace yandex
{
bool RawApi::GetTaxiInfo(ms::LatLon const & from, ms::LatLon const & to, std::string & result)
{
  std::ostringstream url;
  url << std::fixed << std::setprecision(6) << GetYandexURL()
      << "/taxi_info?clid=" << YANDEX_CLIENT_ID << "&apikey=" << YANDEX_API_KEY
      << "&rll=" << from.lon << "," << from.lat << "~" << to.lon << "," << to.lat
      << "&class=econom,business,comfortplus,minivan,vip";

  return RunSimpleHttpRequest(url.str(), result);
}

/// Requests list of available products from Yandex.
void Api::GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                               ProductsCallback const & successFn,
                               ErrorProviderCallback const & errorFn)
{
  ASSERT(successFn, ());
  ASSERT(errorFn, ());

  threads::SimpleThread([from, to, successFn, errorFn]()
  {
    std::string result;
    if (!RawApi::GetTaxiInfo(from, to, result))
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

  }).detach();
}

/// Returns link which allows you to launch the Yandex app.
RideRequestLinks Api::GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                          ms::LatLon const & to) const
{
  std::ostringstream deepLink;
  deepLink << YANDEX_BASE_URL << "%3A%2F%2Froute%3Fstart-lat%3D" << from.lat << "%26start-lon%3D"
           << from.lon << "%26end-lat%3D" << to.lat << "%26end-lon%3D" << to.lon
           << "%26utm_source%3Dmapsme";

  return {deepLink.str(), ""};
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
    int64_t price = 0;
    auto const item = json_array_get(productsArray, i);

    FromJSONObjectOptionalField(item, "waiting_time", time);
    // Temporary unavailable.
    if (time == 0.0)
      continue;

    FromJSONObject(item, "class", product.m_name);
    FromJSONObject(item, "price", price);

    product.m_price = strings::to_string(price);
    product.m_time = strings::to_string(static_cast<int64_t>(time));
    product.m_currency = currency;
    products.push_back(move(product));
  }
}

void SetYandexUrlForTesting(std::string const & url)
{
  g_yandexUrlForTesting = url;
}
}  // namespace yandex
}  // namespace taxi
