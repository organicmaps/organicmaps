#include "partners_api/guides_on_map_api.hpp"

#include "platform/preferred_languages.hpp"

#include "coding/url.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <utility>

#include "3party/jansson/myjansson.hpp"

using namespace base;

namespace
{
void ParseGallery(std::string const & src, guides_on_map::GuidesOnMap & result)
{
  Json root(src.c_str());
  auto const dataArray = json_object_get(root.get(), "data");

  auto const size = json_array_size(dataArray);

  result.reserve(size);
  for (size_t i = 0; i < size; ++i)
  {
    guides_on_map::GuidesNode item;
    auto const obj = json_array_get(dataArray, i);
    auto const pointObj = json_object_get(obj, "point");
    if (!json_is_object(pointObj))
      continue;

    double lat, lon;
    FromJSONObject(pointObj, "lat", lat);
    FromJSONObject(pointObj, "lon", lon);
    item.m_point = mercator::FromLatLon(lat, lon);

    auto const bundleCountsObj = json_object_get(obj, "bundle_counts");
    if (!json_is_object(bundleCountsObj))
      continue;

    FromJSONObject(bundleCountsObj, "sights", item.m_sightsCount);
    FromJSONObject(bundleCountsObj, "outdoor", item.m_outdoorCount);

    auto const extraObj = json_object_get(obj, "extra");
    if (json_is_object(extraObj))
    {
      auto & info = item.m_guideInfo;
      FromJSONObject(extraObj, "server_id", info.m_id);
      FromJSONObject(extraObj, "name", info.m_name);
      FromJSONObject(extraObj, "image_url", info.m_imageUrl);
      FromJSONObjectOptionalField(extraObj, "tag", info.m_tag);
      FromJSONObject(extraObj, "bookmarks_count", info.m_bookmarksCount);
      FromJSONObject(extraObj, "has_track", info.m_hasTrack);
      FromJSONObjectOptionalField(extraObj, "tracks_length", info.m_tracksLength);
      FromJSONObjectOptionalField(extraObj, "tour_duration", info.m_tourDuration);
      FromJSONObjectOptionalField(extraObj, "ascent", info.m_ascent);
    }

    result.emplace_back(std::move(item));
  }
}

std::string MakeGalleryUrl(std::string const & baseUrl, m2::AnyRectD const & viewport, int zoomLevel,
                           std::string const & lang)
{
  // Support empty baseUrl for opensource build.
  if (baseUrl.empty())
    return {};

  url::Params params = {{"zoom_level", strings::to_string(zoomLevel)}, {"locale", lang}};


  auto const toLatLonFormatted = [](m2::PointD const & point)
  {
    auto const latLon = mercator::ToLatLon(point);
    return strings::to_string_dac(latLon.m_lat, 6) + "," + strings::to_string_dac(latLon.m_lon, 6);
  };

  m2::AnyRectD::Corners corners;
  viewport.GetGlobalPoints(corners);

  ASSERT_EQUAL(corners.size(), 4, ());

  params.emplace_back("left_bottom", toLatLonFormatted(corners[0]));
  params.emplace_back("left_top", toLatLonFormatted(corners[1]));
  params.emplace_back("right_top", toLatLonFormatted(corners[2]));
  params.emplace_back("right_bottom", toLatLonFormatted(corners[3]));

  return url::Make(url::Join(baseUrl, "/gallery/v2/map"), params);
}

bool GetGalleryRaw(std::string const & url, platform::HttpClient::Headers const & headers,
                   std::string & result)
{
  platform::HttpClient request(url);
  request.SetTimeout(5 /* timeoutSec */);
  request.SetRawHeaders(headers);

  return request.RunHttpRequest(result);
}
}  // namespace

namespace guides_on_map
{
Api::Api(std::string const & baseUrl)
  : m_baseUrl(baseUrl)
{
}

void Api::SetDelegate(std::unique_ptr<Delegate> delegate)
{
  m_delegate = std::move(delegate);
}

void Api::GetGuidesOnMap(m2::AnyRectD const & viewport, uint8_t zoomLevel,
                         GuidesOnMapCallback const & onSuccess, OnError const & onError) const
{
  auto const url = MakeGalleryUrl(m_baseUrl, viewport, zoomLevel, languages::GetCurrentNorm());
  if (url.empty())
  {
    onSuccess({});
    return;
  }

  auto const headers = m_delegate->GetHeaders();
  GetPlatform().RunTask(Platform::Thread::Network, [url, headers, onSuccess, onError]()
  {
    std::string httpResult;
    if (!GetGalleryRaw(url, headers, httpResult))
    {
      onError();
      return;
    }

    GuidesOnMap result;
    try
    {
      ParseGallery(httpResult, result);
    }
    catch (Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg(), httpResult));
      onError();
      return;
    }

    onSuccess(result);
  });
}
}  // namespace guides_on_map
