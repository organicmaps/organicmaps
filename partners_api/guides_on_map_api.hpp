#pragma once

#include "platform/http_client.hpp"
#include "platform/safe_callback.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "private.h"

#include <cstdint>
#include <string>

namespace guides_on_map
{
struct GalleryItem
{
  struct GuideInfo
  {
    std::string m_id;
    std::string m_name;
    std::string m_imageUrl;
    std::string m_tag;
    uint32_t m_bookmarksCount;
    bool m_hasTrack;
    double m_tracksLength;
    double m_tourDuration;
    int32_t m_ascent;
  };

  m2::PointD m_point;
  uint32_t m_sightsCount = 0;
  uint32_t m_outdoorCount = 0;
  GuideInfo m_guideInfo;
};

using GalleryOnMap = std::vector<GalleryItem>;

using GalleryCallback = platform::SafeCallback<void(GalleryOnMap const & gallery)>;
using OnError = platform::SafeCallback<void()>;

class Api
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual platform::HttpClient::Headers GetHeaders() = 0;
  };

  Api(std::string const & baseUrl = BOOKMARKS_CATALOG_FRONT_URL);

  void SetDelegate(std::unique_ptr<Delegate> delegate);

  void GetGalleryOnMap(m2::RectD const & viewport, uint8_t zoomLevel,
                       GalleryCallback const & onSuccess, OnError const & onError) const;

private:
  std::unique_ptr<Delegate> m_delegate;

  std::string const m_baseUrl;
};
}  // namespace guides_on_map
