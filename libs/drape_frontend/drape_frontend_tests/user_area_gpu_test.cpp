#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "kml/area_data.hpp"

#include "geometry/mercator.hpp"

#include <cmath>
#include <cstdlib>

namespace user_area_tests
{
using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, UserAreas_GpuFillHolesOutlinesAndOrder)
{
  auto const geographicRing = [](m2::RectD const & r)
  {
    kml::AreaRing ring;
    for (auto const & p : {r.LeftBottom(), r.RightBottom(), r.RightTop(), r.LeftTop()})
      ring.push_back({mercator::ToLatLon(p + m2::PointD(90, 90)), {}});
    return ring;
  };
  kml::AreaData red;
  red.m_style = {0xFF000080, 0x000000FF, 4, true, true};
  red.m_polygons.push_back({geographicRing({-80, -80, 80, 80}), {geographicRing({-25, -25, 25, 25})}});
  auto blue = red;
  blue.m_style.m_fillColor = 0x0000FF80;
  blue.m_polygons = {{geographicRing({0, -60, 85, 0}), {}}};
  auto green = red;
  green.m_style.m_fillColor = 0x00FF00FF;
  green.m_style.m_outlineColor = 0x00000080;
  green.m_style.m_outlineWidth = 8;
  green.m_polygons = {{geographicRing({45, -45, 65, -15}), {}}};

  std::vector<drape_ptr<df::UserAreaRenderParams>> areas;
  df::UserAreasToCache toCache;
  for (auto * area : {&red, &blue, &green})
  {
    TEST(kml::NormalizeAreaGeometry(area->m_polygons), ());
    auto & info = areas.emplace_back(df::GenerateAreaRenderInfo(*area));
    df::TessellateUserArea(*info);
    toCache.emplace_back(info.get(), static_cast<float>(areas.size()));
  }

  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Imported polygon areas", 256, 256, [&](auto & f) { f.AddUserAreas(df::TileKey(0, 0, 2), toCache); });
  auto const & image = fixture.GetLastImage();
  auto const expect = [&](int x, int y, int r, int g, int b)
  {
    auto const color = image.pixelColor(x, y);
    TEST_LESS(std::abs(color.red() - r), 3, (x, y, color.red(), r));
    TEST_LESS(std::abs(color.green() - g), 3, (x, y, color.green(), g));
    TEST_LESS(std::abs(color.blue() - b), 3, (x, y, color.blue(), b));
  };
  expect(118, 118, 255, 255, 255);  // Hole, outside the other polygons.
  expect(78, 78, 255, 127, 127);    // Translucent red over white.
  expect(210, 158, 127, 127, 255);  // Translucent blue over white.
  expect(163, 158, 127, 63, 191);   // Blue over red.
  expect(183, 158, 0, 255, 0);      // Later opaque fill must stay above earlier transparent fills.
  expect(174, 144, 0, 127, 0);      // Translucent outline corners must blend only once.
  expect(48, 128, 0, 0, 0);         // Actual polygon outline.
}
}  // namespace user_area_tests
