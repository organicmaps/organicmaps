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
void ParseGallery(std::string const & src, guides_on_map::GalleryOnMap & result)
{
  Json root(src.c_str());
  auto const dataArray = json_object_get(root.get(), "data");

  auto const size = json_array_size(dataArray);

  result.reserve(size);
  for (size_t i = 0; i < size; ++i)
  {
    guides_on_map::GalleryItem item;
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
      FromJSONObject(extraObj, "server_id", item.m_guideInfo.m_id);
      FromJSONObject(extraObj, "name", item.m_guideInfo.m_name);
      FromJSONObject(extraObj, "image_url", item.m_guideInfo.m_imageUrl);
      FromJSONObjectOptionalField(extraObj, "tag", item.m_guideInfo.m_tag);
      FromJSONObject(extraObj, "bookmarks_count", item.m_guideInfo.m_bookmarksCount);
      FromJSONObject(extraObj, "has_track", item.m_guideInfo.m_hasTrack);
      FromJSONObjectOptionalField(extraObj, "tracks_length", item.m_guideInfo.m_tracksLength);
      FromJSONObjectOptionalField(extraObj, "tour_duration", item.m_guideInfo.m_tourDuration);
      FromJSONObjectOptionalField(extraObj, "ascent", item.m_guideInfo.m_ascent);
    }

    result.emplace_back(std::move(item));
  }
}

std::string MakeGalleryUrl(std::string const & baseUrl, m2::RectD const & viewport, int zoomLevel,
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

  params.emplace_back("left_bottom", toLatLonFormatted(viewport.LeftBottom()));
  params.emplace_back("left_top", toLatLonFormatted(viewport.LeftTop()));
  params.emplace_back("right_top", toLatLonFormatted(viewport.RightTop()));
  params.emplace_back("right_bottom", toLatLonFormatted(viewport.RightBottom()));

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

void Api::GetGalleryOnMap(m2::RectD const & viewport, uint8_t zoomLevel,
                          GalleryCallback const & onSuccess, OnError const & onError) const
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

    GalleryOnMap result;
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