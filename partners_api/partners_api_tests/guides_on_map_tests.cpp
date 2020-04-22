#include "testing/testing.hpp"

#include "partners_api/guides_on_map_api.hpp"

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

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
    m2::RectD viewport = {};
    uint8_t zoomlevel = 1;

    guides_on_map::GalleryOnMap result{};
    api.GetGalleryOnMap(viewport, zoomlevel, [&result](guides_on_map::GalleryOnMap const & gallery)
                        {
                          result = gallery;
                          testing::Notify();
                        },
                        []
                        {
                          testing::Notify();
                        });

    testing::Wait();
    TEST_EQUAL(result.size(), 2, ());
  }
  {
    guides_on_map::Api api;
    api.SetDelegate(std::make_unique<DelegateForTesting>());
    m2::PointD leftTop = mercator::FromLatLon(55.781177, 37.564582);
    m2::PointD rightBottom = mercator::FromLatLon(55.725608, 37.699851);
    m2::RectD viewport(leftTop, rightBottom);
    uint8_t zoomlevel = 1;

    guides_on_map::GalleryOnMap result{};
    api.GetGalleryOnMap(viewport, zoomlevel, [&result](guides_on_map::GalleryOnMap const & gallery)
                        {
                          result = gallery;
                          testing::Notify();
                        },
                        []
                        {
                          testing::Notify();
                        });

    testing::Wait();
    TEST(!result.empty(), ());
  }
}
