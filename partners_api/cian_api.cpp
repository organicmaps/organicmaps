#include "partners_api/cian_api.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/logging.hpp"

#include <unordered_set>
#include <utility>

#include "3party/jansson/myjansson.hpp"

using namespace partners_api;
using namespace platform;

namespace
{
// Length of the rect side in meters.
double const kSearchRadius = 500.0;

std::unordered_set<std::string> const kSupportedCities
{
  "Moscow",
  "Saint Petersburg",
  "Nizhny Novgorod",
  "Samara",
  "Kazan",
  "Krasnodar",
  "Rostov-on-Don",
  "Ufa"
};

template <typename T, typename std::enable_if<std::is_integral<T>::value, void>::type* = nullptr>
void GetNullable(json_t * src, std::string const & fieldName, T & result)
{
  result = 0;

  auto const field = json_object_get(src, fieldName.c_str());
  if (field != nullptr && !json_is_null(field))
    result = static_cast<T>(json_integer_value(field));
}

void GetNullable(json_t * src, std::string const & fieldName, std::string & result)
{
  result.clear();

  auto const field = json_object_get(src, fieldName.c_str());
  if (field != nullptr && !json_is_null(field))
    result = json_string_value(field);
}

void MakeResult(std::string const & src, std::vector<cian::RentPlace> & result)
{
  my::Json root(src.c_str());
  if (!json_is_object(root.get()))
    MYTHROW(my::Json::Exception, ("The answer must contain a json object."));

  json_t * clusters = json_object_get(root.get(), "clusters");

  if (clusters == nullptr)
    return;

  size_t const clustersSize = json_array_size(clusters);

  for (size_t i = 0; i < clustersSize; ++i)
  {
    auto cluster = json_array_get(clusters, i);
    cian::RentPlace place;
    FromJSONObject(cluster, "lat", place.m_latlon.lat);
    FromJSONObject(cluster, "lng", place.m_latlon.lon);
    FromJSONObject(cluster, "url", place.m_url);

    json_t * offers = json_object_get(cluster, "offers");

    if (offers == nullptr)
      continue;

    size_t const offersSize = json_array_size(offers);

    if (offersSize == 0)
      continue;

    for (size_t i = 0; i < offersSize; ++i)
    {
      auto offer = json_array_get(offers, i);

      cian::RentOffer rentOffer;

      GetNullable(offer, "flatType", rentOffer.m_flatType);
      GetNullable(offer, "roomsCount", rentOffer.m_roomsCount);
      GetNullable(offer, "priceRur", rentOffer.m_priceRur);
      GetNullable(offer, "floorNumber", rentOffer.m_floorNumber);
      GetNullable(offer, "floorsCount", rentOffer.m_floorsCount);
      GetNullable(offer, "url", rentOffer.m_url);
      GetNullable(offer, "address", rentOffer.m_address);

      if (rentOffer.IsValid())
        place.m_offers.push_back(std::move(rentOffer));
    }

    result.push_back(std::move(place));
  }
}
}  // namespace

namespace cian
{
std::string const kBaseUrl = "https://api.cian.ru/rent-nearby/v1";

// static
http::Result RawApi::GetRentNearby(m2::RectD const & rect,
                                   std::string const & baseUrl /* = kBaseUrl */)
{
  std::ostringstream url;
  url << std::setprecision(6) << baseUrl << "/get-offers-in-bbox/?bbox=" << rect.minX() << ',' << rect.maxY() << '~'
      << rect.maxX() << ',' << rect.minY();

  return http::RunSimpleRequest(url.str());
}

Api::Api(std::string const & baseUrl /* = kBaseUrl */) : m_baseUrl(baseUrl) {}

Api::~Api()
{
  m_worker.Shutdown(base::WorkerThread::Exit::SkipPending);
}

uint64_t Api::GetRentNearby(ms::LatLon const & latlon, RentNearbyCallback const & cb,
                            ErrorCallback const & errCb)
{
  auto const reqId = ++m_requestId;
  auto const & baseUrl = m_baseUrl;

  // Cian supports inverted lon lat coordinates
  auto const mercatorRect = MercatorBounds::MetresToXY(latlon.lat, latlon.lon, kSearchRadius);
  auto const rect = MercatorBounds::ToLatLonRect(mercatorRect);

  m_worker.Push([reqId, rect, cb, errCb, baseUrl]() {
    std::vector<RentPlace> result;

    auto const rawResult = RawApi::GetRentNearby(rect, baseUrl);
    if (!rawResult)
    {
      auto & code = rawResult.m_errorCode;
      GetPlatform().RunOnGuiThread([errCb, code, reqId]() { errCb(code, reqId); });
      return;
    }

    try
    {
      MakeResult(rawResult.m_data, result);
    }
    catch (my::Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg()));
      result.clear();
    }
    GetPlatform().RunOnGuiThread([cb, result, reqId]() { cb(result, reqId); });
  });

  return reqId;
}

// static
bool Api::IsCitySupported(std::string const & city)
{
  return kSupportedCities.find(city) != kSupportedCities.cend();
}
}  // namespace cian
