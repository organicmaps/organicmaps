#include "partners_api/maxim_api.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

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
double const kSecInMinute = 60.;

bool RunSimpleHttpRequest(std::string const & url, std::string & result)
{
  platform::HttpClient request(url);
  return request.RunHttpRequest(result);
}
}  // namespace

namespace taxi
{
namespace maxim
{
std::string const kTaxiInfoUrl = "http://cabinet.taximaxim.ru/Services/Public.svc";

bool RawApi::GetTaxiInfo(ms::LatLon const & from, ms::LatLon const & to, std::string & result,
                         std::string const & baseUrl /* = kTaxiInfoUrl */)
{
  std::ostringstream url;
  url << std::fixed << std::setprecision(6) << baseUrl
      << "/CalculateByCoords?version=1.0&platform=WEB&RefOrgId="
      << MAXIM_CLIENT_ID << "&access-token=" << MAXIM_SERVER_TOKEN
      << "&startLatitude=" << from.lat << "&startLongitude=" << from.lon
      << "&endLatitude=" << to.lat << "&endLongitude=" << to.lon;

  return RunSimpleHttpRequest(url.str(), result);
}

/// Requests list of available products from Maxim.
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
      LOG(LERROR, (e.what(), result));
      products.clear();
    }

    if (products.empty())
      errorFn(ErrorCode::NoProducts);
    else
      successFn(products);

  });
}

/// Returns link which allows you to launch the Maxim app.
RideRequestLinks Api::GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                          ms::LatLon const & to) const
{
  std::ostringstream orderLink;
  std::ostringstream installLink;

  orderLink << "order?refOrgId=" << MAXIM_CLIENT_ID << "&startLatitude=" << from.lat
            << "&startLongitude=" << from.lon << "&endLatitude=" << to.lat
            << "&endLongitude=" << to.lon;

#if defined(OMIM_OS_IPHONE)
  installLink << "https://itunes.apple.com/app/apple-store/id579985456?pt=119057982"
               << "&ct=maps_me&mt=8";
#elif defined(OMIM_OS_ANDROID)
  installLink << "https://play.google.com/store/apps/details?id=com.taxsee.taxsee"
               << "&referrer=utm_source%3Dmaps_me";
#endif

  return {"maximzakaz://" + orderLink.str(), installLink.str()};
}

void MakeFromJson(std::string const & src, std::vector<taxi::Product> & products)
{
  products.clear();

  my::Json root(src.c_str());
  if (!json_is_object(root.get()))
    return;

  bool success = false;
  FromJSONObject(root.get(), "Success", success);

  if (!success)
    return;

  auto const price = json_object_get(root.get(), "Price");
  if (price == nullptr || json_is_null(price))
    return;

  double p = 0.0;
  FromJSON(price, p);
  if (p == 0.0)
    return;

  taxi::Product product;

  FromJSONObject(root.get(), "PriceString", product.m_price);
  FromJSONObject(root.get(), "CurrencyCode", product.m_currency);

  double time = 0.0;
  FromJSONObject(root.get(), "FeedTime", time);
  product.m_time = strings::to_string(static_cast<int64_t>(time * kSecInMinute));

  products.push_back(move(product));
}
}  // namespace maxim
}  // namespace taxi
