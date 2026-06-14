#include "testing/testing.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"

#include "drape/color.hpp"
#include "drape/hatching_decl.hpp"

#include "geometry/point2d.hpp"

#include <string_view>
#include <vector>

namespace area_pattern_gpu_test
{
df::AreaViewParams MakeParams(std::string_view hatching)
{
  df::AreaViewParams p;
  p.m_tileCenter = {0, 0};
  p.m_color = dp::Color(0, 160, 160, 255);  // teal, clearly distinct from the background
  p.m_depth = 0;
  p.m_depthTestEnabled = false;
  p.m_depthLayer = df::DepthLayer::GeometryLayer;
  p.m_minVisibleScale = 0;
  p.m_rank = 0;
  p.m_areaPattern = hatching;
  p.m_baseGtoPScale = 1.0;  // pattern at base scale: crisp 1px features with clear gaps
  return p;
}

// Renders a hatched quad through the real GL pipeline and asserts the analytic pattern behaves: the fill
// shows up on covered fragments, the quad is NOT flooded (gaps exist => it is a pattern, not a solid
// fill), and nothing samples as opaque black. No mask texture is bound, so a binding mistake or a broken
// coverage function would surface here as an empty frame, a flooded quad, or black garbage.
void RenderAndCheck(char const * title, std::string_view hatching)
{
  df::test_support::ShapeTestFixture fixture;
  uint32_t constexpr kW = 256, kH = 256;
  fixture.Render(title, kW, kH, [hatching](df::test_support::ShapeTestFixture & f)
  {
    // Quad covering most of the viewport (two triangles), in world == pixel-from-center coords.
    std::vector<m2::PointD> triangles = {{-110, -110}, {110, -110}, {110, 110}, {-110, -110}, {110, 110}, {-110, 110}};
    f.AddShape(make_unique_dp<df::AreaShape>(std::move(triangles), df::BuildingOutline{}, MakeParams(hatching)));
  });

  QImage const & img = fixture.GetLastImage();
  if (img.isNull())
    return;  // Headless env without a usable GL context - nothing to assert.

  uint32_t teal = 0, opaqueBlack = 0;
  for (int y = 0; y < img.height(); ++y)
  {
    for (int x = 0; x < img.width(); ++x)
    {
      QColor const c = img.pixelColor(x, y);
      if (c.green() > c.red() + 20 && c.blue() > c.red() + 20)  // teal fill on the pattern
        ++teal;
      if (c.alpha() > 200 && c.red() < 8 && c.green() < 8 && c.blue() < 8)  // garbage / unbound colour texture
        ++opaqueBlack;
    }
  }

  TEST_GREATER(teal, 0u, ("Pattern not visible:", title));
  TEST_LESS(teal, kW * kH / 2, ("No gaps - pattern degenerated into a solid fill?", title));
  TEST_EQUAL(opaqueBlack, 0u, ("Opaque black pixels - colour texture not sampled?", title));
}

// A solid-fill pattern (stipple/speckle/...) fills the quad with the surface colour and modulates it with
// darker dots. Validate the fill is present, the speckle is present, and nothing samples as black.
void RenderSolidPatternAndCheck(char const * title, std::string_view patternKey)
{
  df::test_support::ShapeTestFixture fixture;
  uint32_t constexpr kW = 256, kH = 256;
  fixture.Render(title, kW, kH, [patternKey](df::test_support::ShapeTestFixture & f)
  {
    df::AreaViewParams p = MakeParams({});  // no hatch
    p.m_areaPattern = patternKey;
    std::vector<m2::PointD> triangles = {{-110, -110}, {110, -110}, {110, 110}, {-110, -110}, {110, 110}, {-110, 110}};
    f.AddShape(make_unique_dp<df::AreaShape>(std::move(triangles), df::BuildingOutline{}, p));
  });

  QImage const & img = fixture.GetLastImage();
  if (img.isNull())
    return;  // Headless env without a usable GL context - nothing to assert.

  uint32_t fill = 0, dots = 0, opaqueBlack = 0;
  for (int y = 0; y < img.height(); ++y)
  {
    for (int x = 0; x < img.width(); ++x)
    {
      QColor const c = img.pixelColor(x, y);
      bool const teal = c.green() > c.red() + 20 && c.blue() > c.red() + 20;
      if (teal)
        ++fill;
      if (teal && c.green() < 145)  // base teal G=160 -> dots darken it to ~128
        ++dots;
      if (c.alpha() > 200 && c.red() < 8 && c.green() < 8 && c.blue() < 8)
        ++opaqueBlack;
    }
  }

  TEST_GREATER(fill, kW * kH / 4, ("Solid fill not rendered:", title));
  TEST_GREATER(dots, 0u, ("Speckle not visible:", title));
  TEST_EQUAL(opaqueBlack, 0u, ("Opaque black pixels - colour texture not sampled?", title));
}
}  // namespace area_pattern_gpu_test

UNIT_TEST(AreaHatch45GpuTest)
{
  area_pattern_gpu_test::RenderAndCheck("Analytic 45d hatch", dp::k45dHatching);
}

UNIT_TEST(AreaHatchDashGpuTest)
{
  area_pattern_gpu_test::RenderAndCheck("Analytic dash hatch", dp::kDashHatching);
}

UNIT_TEST(AreaStippleGpuTest)
{
  area_pattern_gpu_test::RenderSolidPatternAndCheck("Analytic stipple", dp::kStipplePattern);
}

UNIT_TEST(AreaSpeckleGpuTest)
{
  area_pattern_gpu_test::RenderSolidPatternAndCheck("Analytic speckle", dp::kSpecklePattern);
}

UNIT_TEST(AreaGridGpuTest)
{
  area_pattern_gpu_test::RenderSolidPatternAndCheck("Analytic grid", dp::kGridPattern);
}
