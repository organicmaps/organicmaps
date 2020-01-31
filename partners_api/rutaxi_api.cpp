#include "partners_api/rutaxi_api.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "coding/url.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include <iomanip>
#include <memory>
#include <sstream>
#include <utility>

#include "3party/jansson/myjansson.hpp"

#include "private.h"

namespace
{
std::string const kMappingFilepath = "taxi_places/osm_to_rutaxi.json";
std::string const kArrivalTimeSeconds = "300";

bool RunSimpleHttpRequest(std::string const & url, std::string const & data, std::string & result)
{
  platform::HttpClient request(url);

  request.SetTimeout(10.0);

  if (!data.empty())
    request.SetBodyData(data, "application/json");

  request.SetRawHeader("Accept", "application/json");
  request.SetRawHeader("X-Parse-Application-Id", std::string("App ") + RUTAXI_APP_TOKEN);

  return request.RunHttpRequest(result);
}
}  // namespace

namespace taxi
{
namespace rutaxi
{
std::string const kTaxiInfoUrl = "https://api.rutaxi.ru/api/1.0.0/";

// static
bool RawApi::GetNearObject(ms::LatLon const & pos, std::string const & city, std::string & result,
                           std::string const & baseUrl /* = kTaxiInfoUrl */)
{
  std::ostringstream data;
  data << R"({"latitude": )" << pos.m_lat << R"(, "longitude": )" << pos.m_lon << R"(, "city": ")"
       << city << R"("})";

  return RunSimpleHttpRequest(baseUrl + "near/", data.str(), result);
}

// static
bool RawApi::GetCost(Object const & from, Object const & to, std::string const & city,
                     std::string & result, std::string const & baseUrl /* = kTaxiInfoUrl */)
{
  std::ostringstream data;
  data << R"({"city": ")" << city << R"(", "Order": {"points": [{"object_id": )" << from.m_id
       << R"(, "house": ")" << from.m_house << R"("}, {"object_id": )" << to.m_id
       << R"(, "house": ")" << to.m_house << R"("}]}})";

  return RunSimpleHttpRequest(baseUrl + "cost/", data.str(), result);
}

Api::Api(std::string const & baseUrl /* = kTaxiInfoUrl */)
  : ApiBase(baseUrl)
  , m_cityMapping(LoadCityMapping())
{
}

void Api::SetDelegate(Delegate * delegate)
{
  m_delegate = delegate;
}

void Api::GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                               ProductsCallback const & successFn,
                               ErrorProviderCallback const & errorFn)
{
  ASSERT(successFn, ());
  ASSERT(errorFn, ());
  ASSERT(m_delegate, ());

  auto const fromCity = m_delegate->GetCityName(mercator::FromLatLon(from));
  auto const toCity = m_delegate->GetCityName(mercator::FromLatLon(to));
  auto const cityIdIt = m_cityMapping.find(toCity);

  // TODO(a): Add ErrorCode::FarDistance and provide this error code.
  if (fromCity != toCity || cityIdIt == m_cityMapping.cend() || !IsDistanceSupported(from, to))
  {
    errorFn(ErrorCode::NoProducts);
    return;
  }

  auto const baseUrl = m_baseUrl;
  auto const & city = cityIdIt->second;

  GetPlatform().RunTask(Platform::Thread::Network, [from, to, city, baseUrl, successFn, errorFn]()
  {
    auto const getNearObject = [&city, &baseUrl, &errorFn](ms::LatLon const & pos, Object & dst)
    {
      std::string httpResult;
      if (!RawApi::GetNearObject(pos, city.m_id, httpResult, baseUrl))
      {
        errorFn(ErrorCode::RemoteError);
        return false;
      }

      try
      {
        MakeNearObject(httpResult, dst);
      }
      catch (base::Json::Exception const & e)
      {
        errorFn(ErrorCode::NoProducts);
        LOG(LERROR, (e.what(), httpResult));
        return false;
      }

      return true;
    };

    Object fromObj;
    Object toObj;

    if (!getNearObject(from, fromObj) || !getNearObject(to, toObj))
      return;

    std::string result;
    if (!RawApi::GetCost(fromObj, toObj, city.m_id, result, baseUrl))
    {
      errorFn(ErrorCode::RemoteError);
      return;
    }

    std::vector<Product> products;
    try
    {
      MakeProducts(result, fromObj, toObj, city, products);
    }
    catch (base::Json::Exception const & e)
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

/// Returns link which allows you to launch the RuTaxi app.
RideRequestLinks Api::GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                          ms::LatLon const & to) const
{
  return {"vzt://order.rutaxi.ru/a.php?&" + productId, "https://go.onelink.me/757212956/mapsmevezet"};
}

void MakeNearObject(std::string const & src, Object & dst)
{
  base::Json root(src.c_str());

  auto const data = json_object_get(root.get(), "data");
  auto const objects = json_object_get(data, "objects");
  auto const item = json_array_get(objects, 0);

  FromJSONObject(item, "id", dst.m_id);
  FromJSONObject(item, "house", dst.m_house);
  FromJSONObject(item, "name", dst.m_title);
}

void MakeProducts(std::string const & src, Object const & from, Object const & to,
                  City const & city, std::vector<taxi::Product> & products)
{
  products.clear();

  base::Json root(src.c_str());

  std::ostringstream productStream;
// Vezet app for Android is not decodes url encoding.
#if defined(OMIM_OS_ANDROID)
  productStream << "city=" << city.m_id << "&title1=" << from.m_title
                << "&ob1=" << from.m_id << "&h1=" << from.m_house
                << "&title2=" << to.m_title << "&ob2=" << to.m_id
                << "&h2=" << to.m_house << "&e1=";
#else
  productStream << "city=" << city.m_id << "&title1=" << url::UrlEncode(from.m_title)
                << "&ob1=" << from.m_id << "&h1=" << url::UrlEncode(from.m_house)
                << "&title2=" << url::UrlEncode(to.m_title) << "&ob2=" << to.m_id
                << "&h2=" << url::UrlEncode(to.m_house) << "&e1=";
#endif

  taxi::Product product;
  product.m_productId = productStream.str();
  product.m_currency = city.m_currency;
  product.m_time = kArrivalTimeSeconds;

  auto const data = json_object_get(root.get(), "data");

  FromJSONObject(data, "cost", product.m_price);

  products.emplace_back(std::move(product));
}

CityMapping LoadCityMapping()
{
  std::string fileData;
  try
  {
    auto const fileReader = GetPlatform().GetReader(kMappingFilepath);
    fileReader->ReadAsString(fileData);
  }
  catch (FileAbsentException const & ex)
  {
    LOG(LERROR, ("Exception while get reader for file:", kMappingFilepath, "reason:", ex.what()));
    return {};
  }
  catch (FileReader::Exception const & ex)
  {
    LOG(LERROR, ("Exception while reading file:", kMappingFilepath, "reason:", ex.what()));
    return {};
  }

  ASSERT(!fileData.empty(), ());

  CityMapping result;

  try
  {
    base::Json root(fileData.c_str());

    auto const count = json_array_size(root.get());
    std::string osmName;
    City city;

    for (size_t i = 0; i < count; ++i)
    {
      auto const item = json_array_get(root.get(), i);

      FromJSONObject(item, "osm", osmName);
      FromJSONObject(item, "rutaxi", city.m_id);
      FromJSONObject(item, "currency", city.m_currency);

      result.emplace(osmName, city);
    }
  }
  catch (base::Json::Exception const & ex)
  {
    LOG(LWARNING, ("Exception while parsing file:", kMappingFilepath, "reason:", ex.what(),
                   "json:", fileData));
    return {};
  }

  return result;
}
}  // namespace rutaxi
}  // namespace taxi
