#pragma once

#include "platform/http_client.hpp"
#include "platform/safe_callback.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/point2d.hpp"

#include "base/task_loop.hpp"

#include "private.h"

#include <cstdint>
#include <memory>
#include <string>

namespace guides_on_map
{
struct GuidesNode
{
  struct GuideInfo
  {
    // |id| contains guide id on the server.
    std::string m_id;
    std::string m_name;
    std::string m_imageUrl;
    std::string m_tag;
    uint32_t m_bookmarksCount = 0;
    bool m_hasTrack = false;
    // Length in meters.
    double m_tracksLength = 0.0;
    // Duration in seconds.
    double m_tourDuration = 0.0;
    // Ascent in meters.
    int32_t m_ascent = 0;
  };

  m2::PointD m_point;
  uint32_t m_sightsCount = 0;
  uint32_t m_outdoorCount = 0;
  GuideInfo m_guideInfo;
};

struct GuidesOnMap
{
  static uint8_t constexpr kIncorrectZoom = 0;

  std::vector<GuidesNode> m_nodes;
  uint8_t m_suggestedZoom = kIncorrectZoom;
};

using GuidesOnMapCallback = platform::SafeCallback<void(GuidesOnMap const & gallery)>;
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

  base::TaskLoop::PushResult GetGuidesOnMap(m2::AnyRectD::Corners const & corners,
                                            uint8_t zoomLevel, bool suggestZoom,
                                            uint8_t rectIncreasedPercent,
                                            GuidesOnMapCallback const & onSuccess,
                                            OnError const & onError) const;

private:
  std::unique_ptr<Delegate> m_delegate;

  std::string const m_baseUrl;
};
}  // namespace guides_on_map
