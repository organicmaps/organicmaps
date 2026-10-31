#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "kml/area_data.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <cmath>
#include <cstdlib>

namespace user_area_tests
{
using df::test_support::VisualParamsFixture;

// A ring in the coordinates of the rendered tile, centered at 0.
kml::AreaRing GeographicRing(m2::RectD const & r)
{
  kml::AreaRing ring;
  for (auto const & p : {r.LeftBottom(), r.RightBottom(), r.RightTop(), r.LeftTop()})
    ring.push_back({mercator::ToLatLon(p + m2::PointD(90, 90)), {}});
  return ring;
}

UNIT_CLASS_TEST(VisualParamsFixture, UserAreas_GpuFillHolesOutlinesAndOrder)
{
  kml::AreaData red;
  red.m_style = {0xFF000080, 0x000000FF, 4, true, true};
  red.m_polygons.push_back({GeographicRing({-80, -80, 80, 80}), {GeographicRing({-25, -25, 25, 25})}});
  auto blue = red;
  blue.m_style.m_fillColor = 0x0000FF80;
  blue.m_polygons = {{GeographicRing({0, -60, 85, 0}), {}}};
  auto green = red;
  green.m_style.m_fillColor = 0x00FF00FF;
  green.m_style.m_outlineColor = 0x00000080;
  green.m_style.m_outlineWidth = 8;
  green.m_polygons = {{GeographicRing({45, -45, 65, -15}), {}}};

  std::vector<drape_ptr<df::UserAreaRenderParams>> areas;
  df::UserAreasToCache toCache;
  for (auto * area : {&red, &blue, &green})
  {
    TEST(kml::NormalizeAreaGeometry(area->m_polygons), ());
    auto & info = areas.emplace_back(df::GenerateAreaRenderInfo(*area));
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

UNIT_CLASS_TEST(VisualParamsFixture, UserAreas_GpuTranslucentOutlinesBlendInPaintOrder)
{
  kml::AreaData red;
  red.m_style = {0xFFFFFF00, 0xFF000080, 16, false, true};
  red.m_polygons.push_back({GeographicRing({-60, -60, 0, 0}), {GeographicRing({-45, -45, -15, -15})}});
  auto blue = red;
  blue.m_style.m_outlineColor = 0x0000FF80;
  blue.m_polygons = {{GeographicRing({6, -40, 60, 40}), {}}};

  std::vector<drape_ptr<df::UserAreaRenderParams>> areas;
  df::UserAreasToCache toCache;
  for (auto * area : {&red, &blue})
  {
    TEST(kml::NormalizeAreaGeometry(area->m_polygons), ());
    auto & info = areas.emplace_back(df::GenerateAreaRenderInfo(*area));
    toCache.emplace_back(info.get(), static_cast<float>(areas.size()));
  }

  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Translucent area outlines", 256, 256,
                 [&](auto & f) { f.AddUserAreas(df::TileKey(0, 0, 2), toCache); });
  auto const & image = fixture.GetLastImage();
  auto const expect = [&](int x, int y, int r, int g, int b)
  {
    auto const color = image.pixelColor(x, y);
    TEST_LESS(std::abs(color.red() - r), 3, (x, y, color.red(), r));
    TEST_LESS(std::abs(color.green() - g), 3, (x, y, color.green(), g));
    TEST_LESS(std::abs(color.blue() - b), 3, (x, y, color.blue(), b));
  };
  expect(131, 125, 127, 63, 191);   // The later blue outline crosses the round join of the red outer corner.
  expect(65, 125, 255, 127, 127);   // A join of a left turn alone.
  expect(116, 140, 255, 127, 127);  // A join of a right turn, at a corner of the hole, alone.
}

UNIT_CLASS_TEST(VisualParamsFixture, UserAreas_GpuSmallRingOutlineIsVisible)
{
  // The ring is smaller than a pixel and than the outline simplification step, while its outline is visible.
  kml::AreaData area;
  area.m_style = {0xFFFFFF00, 0x000000FF, 8, false, true};
  area.m_polygons.push_back({GeographicRing({-0.1, -0.1, 0.1, 0.1}), {}});
  TEST(kml::NormalizeAreaGeometry(area.m_polygons), ());
  auto const info = df::GenerateAreaRenderInfo(area);

  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Small area outline", 256, 256,
                 [&](auto & f) { f.AddUserAreas(df::TileKey(0, 0, 2), {{info.get(), 1.0f}}); });
  auto const & image = fixture.GetLastImage();
  int colored = 0;
  for (int y = 0; y < image.height(); ++y)
    for (int x = 0; x < image.width(); ++x)
      colored += image.pixelColor(x, y) != QColor(Qt::white);
  TEST_GREATER(colored, 0, ());
}

UNIT_CLASS_TEST(VisualParamsFixture, UserAreas_GpuBucketOverflowKeepsFillBelowOutline)
{
  // A circle with a 60 px radius at a zoom without outline simplification, so that its outline bucket fills up
  // before the fill one.
  df::TileKey const tile(0, 0, 16);
  auto const center = tile.GetWrappedDataRect().Center();
  double const pixel = tile.GetWrappedDataRect().SizeX() / 256;
  kml::AreaData area;
  area.m_style = {0x00FF00FF, 0x00000080, 20, true, true};
  auto & ring = area.m_polygons.emplace_back().m_outer;
  for (int i = 0; i < 1200; ++i)
  {
    double const angle = 2 * math::pi * i / 1200;
    ring.push_back({mercator::ToLatLon(center + m2::PointD(std::cos(angle), std::sin(angle)) * (60 * pixel)), {}});
  }
  auto const info = df::GenerateAreaRenderInfo(area);

  df::test_support::ShapeTestFixture fixture;
  fixture.SetWorldToPixelScale(static_cast<float>(1 / pixel));
  fixture.Render("Area buckets keep the blending order", 256, 256,
                 [&](auto & f) { f.AddUserAreas(tile, {{info.get(), 1.0f}}); });
  auto const color = fixture.GetLastImage().pixelColor(183, 128);
  TEST_LESS(color.red(), 3, (color.red()));
  TEST_LESS(std::abs(color.green() - 127), 3, (color.green()));
  TEST_LESS(color.blue(), 3, (color.blue()));
}

UNIT_CLASS_TEST(VisualParamsFixture, UserAreas_GpuOnlyPolygonsCrossingTilesAreTessellated)
{
  auto const ring = [](m2::RectD const & r)
  {
    kml::AreaRing ring;
    for (auto const & p : {r.LeftBottom(), r.RightBottom(), r.RightTop(), r.LeftTop()})
      ring.push_back({mercator::ToLatLon(p), {}});
    return ring;
  };
  // Polygons around the tile, in it and beyond it.
  df::TileKey const tile(0, 0, 3);
  auto around = tile.GetWrappedDataRect();
  around.Inflate(10, 10);
  kml::AreaData area;
  area.m_style = {0xFF0000FF, 0, 0, true, false};
  area.m_polygons = {{ring(around), {}}, {ring({10, 10, 20, 20}), {}}, {ring({120, 120, 130, 130}), {}}};
  for (bool const hole : {false, true})
  {
    if (hole)
    {
      auto inner = tile.GetWrappedDataRect();
      inner.Inflate(5, 5);
      area.m_polygons.front().m_holes.push_back(ring(inner));
    }
    auto const info = df::GenerateAreaRenderInfo(area);

    df::test_support::ShapeTestFixture fixture;
    fixture.Render("Lazy area tessellation", 256, 256, [&](auto & f) { f.AddUserAreas(tile, {{info.get(), 1.0f}}); });
    auto const & image = fixture.GetLastImage();
    auto const color = image.pixelColor(128, 128);
    TEST(color == (hole ? QColor(Qt::white) : QColor(Qt::red)), (hole, color.red(), color.green(), color.blue()));
    TEST(image.pixelColor(98, 158) == QColor(Qt::red), (hole));
    // Only the polygon whose ring is in the tile needs triangles.
    TEST(info->m_triangles[0].empty() && !info->m_fillRings[0].empty(), (hole));
    TEST(!info->m_triangles[1].empty(), (hole));
    TEST(info->m_triangles[2].empty() && !info->m_fillRings[2].empty(), (hole));
  }
}
}  // namespace user_area_tests
