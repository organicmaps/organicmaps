#include "partners_api/citymobil_api.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "geometry/latlon.hpp"

#include "coding/url.hpp"
#include "coding/writer.hpp"

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
bool RunHttpRequest(std::string const & url, std::string && body, std::string & result)
{
  platform::HttpClient request(url);
  request.SetRawHeader("Accept", "application/json");
  request.SetRawHeader("Authorization", std::string("Bearer ") + CITYMOBIL_TOKEN);
  request.SetBodyData(std::move(body), "application/json");
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    result = request.ServerResponse();
    return true;
  }

  return false;
}

template<typename T>
std::string SerializeToJson(T const & data)
{
  std::string jsonStr;
  using Sink = MemWriter<std::string>;
  Sink sink(jsonStr);
  coding::SerializerJson<Sink> serializer(sink);
  serializer(data);
  return jsonStr;
}
}  // namespace

namespace taxi
{
namespace citymobil
{
std::string const kBaseUrl = "https://corp-api.city-mobil.ru";

// static
bool RawApi::GetSupportedTariffs(SupportedTariffsBody const & body, std::string & result,
                                std::string const & baseUrl /* = kBaseUrl */)
{
  return RunHttpRequest(url::Join(baseUrl, "get_supported_tariffs"), SerializeToJson(body), result);
}

// static
bool RawApi::CalculatePrice(CalculatePriceBody const & body, std::string & result,
                            std::string const & baseUrl /* = kBaseUrl */)
{
  return RunHttpRequest(url::Join(baseUrl, "calculate_price"), SerializeToJson(body), result);
}

/// Requests list of available products from Citymobil.
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
    std::string tariffsResult;
    RawApi::SupportedTariffsBody supportedTariffs(from);
    if (!RawApi::GetSupportedTariffs(supportedTariffs, tariffsResult, baseUrl))
    {
      errorFn(ErrorCode::RemoteError);
      return;
    }

    RawApi::TariffGroups tariffGroups = MakeTariffGroupsFromJson(tariffsResult);

    std::string calculatePriceResult;
    RawApi::CalculatePriceBody calculatePrice(from, to, tariffGroups);
    if (!RawApi::CalculatePrice(calculatePrice, calculatePriceResult, baseUrl))
    {
      errorFn(ErrorCode::RemoteError);
      return;
    }

    std::vector<Product> products;
    try
    {
      products = MakeProductsFromJson(calculatePriceResult);
    }
    catch (base::Json::Exception const & e)
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

/// Returns link which allows you to launch the Citymobil app.
RideRequestLinks Api::GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                          ms::LatLon const & to) const
{
  std::ostringstream deepLink;
  deepLink << std::fixed << std::setprecision(6) << "https://trk.mail.ru/c/q4akt6?from="
           << from.m_lat << "," << from.m_lon << "&to=" << to.m_lat << "," << to.m_lon
           << "&tariff=" << productId;

  return {deepLink.str(), deepLink.str()};
}

RawApi::TariffGroups MakeTariffGroupsFromJson(std::string const & src)
{
  RawApi::TariffGroups result;
  base::Json root(src.c_str());
  auto const tariffGroups = json_object_get(root.get(), "tariff_groups");
  auto const tariffGroupsSize = json_array_size(tariffGroups);
  result.resize(tariffGroupsSize);

  for (size_t i = 0; i < tariffGroupsSize; ++i)
  {
    auto const item = json_array_get(tariffGroups, i);
    FromJSONObject(item, "tariff_group_id", result[i]);
  }

  return result;
}

std::vector<taxi::Product> MakeProductsFromJson(std::string const & src)
{
  std::vector<taxi::Product> products;

  base::Json root(src.c_str());

  uint32_t serviceStatus;
  FromJSONObject(root.get(), "service_status", serviceStatus);
  // Temporary unavailable.
  if (serviceStatus == 0)
    return {};

  uint32_t eta = 0;
  FromJSONObject(root.get(), "eta", eta);

  auto const productsArray = json_object_get(root.get(), "prices");
  auto const productsSize = json_array_size(productsArray);
  for (size_t i = 0; i < productsSize; ++i)
  {
    taxi::Product product;
    uint32_t id = 0;
    uint32_t price = 0;
    auto const item = json_array_get(productsArray, i);

    FromJSONObject(item, "id_tariff_group", id);
    FromJSONObject(item, "total_price", price);

    auto const tariffInfo = json_object_get(item, "tariff_info");
    FromJSONObject(tariffInfo, "name", product.m_name);

    product.m_productId = strings::to_string(id);
    product.m_price = strings::to_string(price);
    product.m_time = strings::to_string(eta);
    product.m_currency = "RUB";
    products.push_back(std::move(product));
  }

  return products;
}
}  // namespace citymobil
}  // namespace taxi
