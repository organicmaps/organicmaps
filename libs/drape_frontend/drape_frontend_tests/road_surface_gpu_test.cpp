#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/line_shape.hpp"

#include "drape/hatching_decl.hpp"

namespace road_surface_gpu_test
{
// A gap exposes both end caps of the textured stroke next to the plain stroke.
UNIT_TEST(RoadSurface_GrainTransition)
{
  for (auto const base : {230, 48})
  {
    df::test_support::ShapeTestFixture fixture;
    fixture.Render("road-surface", 256, 128, [&](auto & f)
    {
      df::LineViewParams p;
      p.m_color = dp::Color(base, base, base, 255);
      p.m_width = 20;
      p.m_cap = dp::RoundCap;
      p.m_join = dp::RoundJoin;
      p.m_depth = 0;
      p.m_depthTestEnabled = false;
      p.m_depthLayer = df::DepthLayer::GeometryLayer;
      p.m_tileCenter = {0, 0};
      auto add = [&](double x1, double x2)
      {
        m2::SharedSpline spline(std::vector<m2::PointD>{{x1, 0}, {x2, 0}});
        f.AddShape(make_unique_dp<df::LineShape>(spline, p));
      };
      add(-110, -16);
      p.m_grainScale = 1.0f / (dp::kPatternTileSize * df::kShapeCoordScalar);
      add(12, 110);
    });

    auto const & img = fixture.GetLastImage();
    int grains = 0, plain = 0;
    for (int x = 35; x < 105; ++x)
      for (int y = 58; y < 70; ++y)
      {
        auto const paved = qRed(img.pixel(x, y));
        TEST_EQUAL(paved, base, ());
        auto const unpaved = qRed(img.pixel(x + 115, y));
        if (base > 128 ? unpaved < paved - 4 : unpaved > paved + 4)
          ++grains;
        if (unpaved == paved)
          ++plain;
        TEST_EQUAL(img.pixel(x, y + 20), img.pixel(x + 115, y + 20), ());
      }
    // The textured stroke runs from x=140 to x=238, with a 10-pixel semicircle at each end.
    TEST_EQUAL(qRed(img.pixel(14, 64)), base, ());
    TEST_NOT_EQUAL(img.pixel(134, 64), img.pixel(126, 64), ());
    TEST_NOT_EQUAL(img.pixel(242, 64), img.pixel(252, 64), ());
    TEST_EQUAL(img.pixel(252, 64), img.pixel(4, 64), ());
    TEST_GREATER(grains, 20, (base));
    TEST_GREATER(plain, 20, (base));
  }
}
UNIT_TEST(RoadSurface_TranslucentTrackBend)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("paved-track-bend", 128, 128, [](auto & f)
  {
    df::LineViewParams p;
    p.m_color = dp::Color(99, 87, 48, 204);
    p.m_width = 12;
    p.m_cap = dp::RoundCap;
    p.m_join = dp::RoundJoin;
    p.m_blendInPaintOrder = true;
    p.m_tileCenter = {0, 0};
    m2::SharedSpline spline(std::vector<m2::PointD>{{-40, 0}, {0, 0}, {0, 40}});
    f.AddShape(make_unique_dp<df::LineShape>(spline, p));
  });

  auto const & image = fixture.GetLastImage();
  auto const straight = image.pixel(40, 64);
  TEST_NOT_EQUAL(straight, image.pixel(10, 64), ());
  TEST_EQUAL(image.pixel(21, 64), straight, ());  // Start cap.
  for (int x = 60; x < 64; ++x)
    for (int y = 60; y < 64; ++y)
      TEST_EQUAL(image.pixel(x, y), straight, (x, y));  // Overlapping segment rectangles at the bend.
}
}  // namespace road_surface_gpu_test
