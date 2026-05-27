#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/line_shape.hpp"

#include "drape/color.hpp"

#include "geometry/point2d.hpp"

namespace rainbow_line_gpu_test
{
m2::SharedSpline MakeSpline(std::vector<m2::PointD> const & points)
{
  return m2::SharedSpline(points);
}

df::LineViewParams MakeLineParams(dp::Color const & color, float width)
{
  df::LineViewParams params;
  params.m_tileCenter = {0, 0};
  params.m_color = color;
  params.m_width = width;
  params.m_depth = 0;
  params.m_depthTestEnabled = false;
  params.m_depthLayer = df::DepthLayer::GeometryLayer;
  params.m_minVisibleScale = 1;
  params.m_rank = 0;
  params.m_cap = dp::RoundCap;
  params.m_join = dp::RoundJoin;
  params.m_baseGtoPScale = 1.0;
  params.m_zoomLevel = 15;
  return params;
}

df::LineViewParams MakeRainbowParams(dp::RainbowColors const & colors, float stripeWidth)
{
  auto params = MakeLineParams(colors[0], stripeWidth * static_cast<float>(colors.size()));
  params.m_rainbowColors = colors;
  return params;
}
}  // namespace rainbow_line_gpu_test

// Renders rainbow lines through the real GPU shader pipeline using the texture-based approach.
// On headless Linux, set QT_QPA_PLATFORM=offscreen environment variable.
UNIT_TEST(RainbowLineGpuTest)
{
  using namespace rainbow_line_gpu_test;

  df::test_support::ShapeTestFixture fixture;
  fixture.Render("RainbowLine GPU (texture-based)", 400, 300, [](df::test_support::ShapeTestFixture & f)
  {
    // -- 2-color horizontal rainbow line --
    {
      auto spline = MakeSpline({{-180, 100}, {180, 100}});
      auto params = MakeRainbowParams({dp::Color(220, 50, 50, 255), dp::Color(50, 50, 220, 255)}, 6);
      f.AddShape(make_unique_dp<df::LineShape>(spline, params));
    }

    // -- 3-color horizontal rainbow line --
    {
      auto spline = MakeSpline({{-180, 50}, {180, 50}});
      auto params =
          MakeRainbowParams({dp::Color(220, 50, 50, 255), dp::Color(50, 180, 50, 255), dp::Color(50, 50, 220, 255)}, 5);
      f.AddShape(make_unique_dp<df::LineShape>(spline, params));
    }

    // -- 4-color diagonal rainbow line --
    {
      auto spline = MakeSpline({{-180, -20}, {180, -80}});
      auto params = MakeRainbowParams({dp::Color(220, 50, 50, 255), dp::Color(50, 180, 50, 255),
                                       dp::Color(50, 50, 220, 255), dp::Color(220, 180, 50, 255)},
                                      4);
      f.AddShape(make_unique_dp<df::LineShape>(spline, params));
    }

    // -- Base gray line with 3-color rainbow overlay (simulating real usage) --
    {
      auto spline = MakeSpline({{-180, 0}, {180, 0}});
      f.AddShape(make_unique_dp<df::LineShape>(spline, MakeLineParams(dp::Color(180, 180, 180, 255), 14)));
      auto params =
          MakeRainbowParams({dp::Color(220, 50, 50, 255), dp::Color(50, 180, 50, 255), dp::Color(50, 50, 220, 255)}, 4);
      params.m_depth = 1;
      f.AddShape(make_unique_dp<df::LineShape>(spline, params));
    }

    // -- 4-color polyline with concave and convex angles --
    {
      auto spline = MakeSpline({{-180, -100}, {-60, -130}, {60, -100}, {180, -130}});
      auto params = MakeRainbowParams({dp::Color(220, 50, 50, 255), dp::Color(50, 180, 50, 255),
                                       dp::Color(50, 50, 220, 255), dp::Color(220, 180, 50, 255)},
                                      5);
      f.AddShape(make_unique_dp<df::LineShape>(spline, params));
    }
  });
}
