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

df::RainbowLineViewParams MakeRainbowParams(std::vector<dp::Color> const & colors, float stripeWidth)
{
  df::RainbowLineViewParams params;
  params.m_tileCenter = {0, 0};
  params.m_depth = 0;
  params.m_depthTestEnabled = false;
  params.m_depthLayer = df::DepthLayer::GeometryLayer;
  params.m_minVisibleScale = 1;
  params.m_rank = 0;
  for (auto const & c : colors)
    params.m_colors.push_back(c);
  params.m_stripeWidth = stripeWidth;
  params.m_cap = dp::RoundCap;
  params.m_join = dp::RoundJoin;
  params.m_baseGtoPScale = 1.0;
  params.m_zoomLevel = 15;
  return params;
}

df::LineViewParams MakeLineParams(dp::Color const & color, float width)
{
  df::LineViewParams params;
  params.m_tileCenter = {0, 0};
  params.m_depth = -1;
  params.m_depthTestEnabled = false;
  params.m_depthLayer = df::DepthLayer::GeometryLayer;
  params.m_minVisibleScale = 1;
  params.m_rank = 0;
  params.m_color = color;
  params.m_width = width;
  params.m_cap = dp::RoundCap;
  params.m_join = dp::RoundJoin;
  params.m_baseGtoPScale = 1.0;
  params.m_zoomLevel = 15;
  return params;
}

// Renders rainbow lines through the real GPU shader pipeline.
UNIT_TEST(RainbowLineGpuTest)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Init(400, 300);

  // Coordinates in shape space: kShapeCoordScalar maps tile-local coords.
  // With identity modelView + ortho projection, these map directly to pixels from center.

  // -- 2-color horizontal rainbow line --
  {
    auto spline = MakeSpline({{-180, 100}, {180, 100}});
    auto params = MakeRainbowParams({dp::Color(220, 50, 50, 255), dp::Color(50, 50, 220, 255)}, 6);
    fixture.AddShape(make_unique_dp<df::RainbowLineShape>(spline, params));
  }

  // -- 3-color horizontal rainbow line --
  {
    auto spline = MakeSpline({{-180, 50}, {180, 50}});
    auto params =
        MakeRainbowParams({dp::Color(220, 50, 50, 255), dp::Color(50, 180, 50, 255), dp::Color(50, 50, 220, 255)}, 5);
    fixture.AddShape(make_unique_dp<df::RainbowLineShape>(spline, params));
  }

  // -- 4-color diagonal rainbow line --
  {
    auto spline = MakeSpline({{-180, -20}, {180, -80}});
    auto params = MakeRainbowParams({dp::Color(220, 50, 50, 255), dp::Color(50, 180, 50, 255),
                                     dp::Color(50, 50, 220, 255), dp::Color(220, 180, 50, 255)},
                                    4);
    fixture.AddShape(make_unique_dp<df::RainbowLineShape>(spline, params));
  }

  // -- Base line underneath a rainbow (simulating real usage) --
  {
    auto spline = MakeSpline({{-180, 0}, {180, 0}});
    // Gray base line
    fixture.AddShape(make_unique_dp<df::LineShape>(spline, MakeLineParams(dp::Color(180, 180, 180, 255), 14)));
    // Rainbow overlay
    auto params =
        MakeRainbowParams({dp::Color(220, 50, 50, 255), dp::Color(50, 180, 50, 255), dp::Color(50, 50, 220, 255)}, 4);
    params.m_depth = 1;
    fixture.AddShape(make_unique_dp<df::RainbowLineShape>(spline, params));
  }

  fixture.Flush();
  fixture.ShowInWindow("RainbowLine GPU", false /* autoExit */);
}

}  // namespace rainbow_line_gpu_test
