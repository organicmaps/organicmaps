#include "partners_api/cian_api.hpp"
#include "partners_api/utils.hpp"

#include "platform/platform.hpp"

#include "geometry/rect2d.hpp"

#include "base/logging.hpp"

#include <utility>

#include "3party/jansson/myjansson.hpp"

using namespace platform;

namespace
{
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

      FromJSONObject(offer, "flatType", rentOffer.m_flatType);
      FromJSONObject(offer, "roomsCount", rentOffer.m_roomsCount);
      FromJSONObject(offer, "priceRur", rentOffer.m_priceRur);
      FromJSONObject(offer, "floorNumber", rentOffer.m_floorNumber);
      FromJSONObject(offer, "floorsCount", rentOffer.m_floorsCount);
      FromJSONObject(offer, "url", rentOffer.m_url);
      FromJSONObject(offer, "address", rentOffer.m_address);

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
bool RawApi::GetRentNearby(m2::RectD const & rect, std::string & result,
                           std::string const & baseUrl /* = kBaseUrl */)
{
  std::ostringstream url;
  url << baseUrl << "/get-offers-in-bbox/?bbox=" << rect.minX() << ',' << rect.maxY() << '~'
      << rect.maxX() << ',' << rect.minY();

  return partners_api_utils::RunSimpleHttpRequest(url.str(), result);
}

Api::Api(std::string const & baseUrl /* = kBaseUrl */) : m_baseUrl(baseUrl) {}

Api::~Api()
{
  m_worker.Shutdown(base::WorkerThread::Exit::SkipPending);
}

uint64_t Api::GetRentNearby(m2::RectD const & rect, RentNearbyCallback const & cb)
{
  auto const reqId = ++m_requestId;
  auto const baseUrl = m_baseUrl;

  m_worker.Push([reqId, rect, cb, baseUrl]() {
    std::string rawResult;
    std::vector<RentPlace> result;

    if (!RawApi::GetRentNearby(rect, rawResult, baseUrl))
    {
      GetPlatform().RunOnGuiThread([cb, result, reqId]() { cb(result, reqId); });
      return;
    }

    try
    {
      MakeResult(rawResult, result);
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
}  // namespace cian
