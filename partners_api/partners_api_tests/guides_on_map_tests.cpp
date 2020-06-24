#include "testing/testing.hpp"

#include "partners_api/guides_on_map_api.hpp"

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/any_rect2d.hpp"

#include <memory>

using namespace platform::tests_support;

namespace
{
class DelegateForTesting : public guides_on_map::Api::Delegate
{
public:
  platform::HttpClient::Headers GetHeaders() override { return {}; }
};
}  // namespace

UNIT_CLASS_TEST(AsyncGuiThread, GuidesOnMap_GetGalleryOnMap)
{
  {
    guides_on_map::Api api("http://localhost:34568/");
    api.SetDelegate(std::make_unique<DelegateForTesting>());
    m2::AnyRectD::Corners corners = {};
    uint8_t zoomLevel = 1;

    guides_on_map::GuidesOnMap result{};
    api.GetGuidesOnMap(corners, zoomLevel, true, 0, [&result](guides_on_map::GuidesOnMap const & gallery)
                        {
                          result = gallery;
                          testing::Notify();
                        },
                        []
                        {
                          testing::Notify();
                        });

    testing::Wait();
    TEST_EQUAL(result.m_nodes.size(), 2, ());
    TEST_EQUAL(result.m_suggestedZoom, 5, ());
  }

  {
    guides_on_map::Api api;
    api.SetDelegate(std::make_unique<DelegateForTesting>());
    m2::PointD leftTop = mercator::FromLatLon(55.781177, 37.564582);
    m2::PointD rightBottom = mercator::FromLatLon(55.725608, 37.699851);
    m2::RectD rect(leftTop, rightBottom);
    m2::AnyRectD viewport(rect);
    m2::AnyRectD::Corners corners;
    viewport.GetGlobalPoints(corners);
    uint8_t zoomLevel = 10;

    guides_on_map::GuidesOnMap result{};
    api.GetGuidesOnMap(corners, zoomLevel, true, 0, [&result](guides_on_map::GuidesOnMap const & gallery)
                        {
                          result = gallery;
                          testing::Notify();
                        },
                        []
                        {
                          testing::Notify();
                        });

    testing::Wait();
    TEST(!result.m_nodes.empty(), ());
  }
}
