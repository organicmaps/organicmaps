#include "testing/testing.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"

#include "drape/color.hpp"
#include "drape/hatching_decl.hpp"

#include "geometry/point2d.hpp"

#include <string_view>
#include <utility>
#include <vector>

namespace area_pattern_gpu_test
{
// Teal-ish fills with a luma far above and below the 0.5 split between darker and lighter pattern dots. The dark one
// is near-black, where a dot has to lighten the fill by more than a multiplier could.
dp::Color constexpr kLightFill(196, 233, 239, 255);
dp::Color constexpr kDarkFill(2, 25, 25, 255);

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
// fill), and every pixel lands on the straight-alpha blend of the fill over the background. No mask
// texture is bound, so a binding mistake, a broken coverage function or coverage applied to rgb as well
// would surface here as an empty frame, a flooded quad, or a fill darker than the blend allows.
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

  // Blending is straight alpha, so the teal fill over the white clear must land on the white->teal
  // segment: r = 255*(1 - a) and g = b = 255 - 95*a, i.e. g = 255 - 95*(255 - r)/255 whatever the
  // coverage is. Scaling rgb by coverage as well (or sampling an unbound colour texture) can only drag g
  // below that line - by up to 40 levels at a = 0.5, far above the 8-bit rounding tolerance.
  uint32_t teal = 0, tooDark = 0;
  for (int y = 0; y < img.height(); ++y)
  {
    for (int x = 0; x < img.width(); ++x)
    {
      QColor const c = img.pixelColor(x, y);
      if (c.green() > c.red() + 20 && c.blue() > c.red() + 20)  // teal fill on the pattern
        ++teal;
      if (c.green() + 3 < 255 - 95 * (255 - c.red()) / 255)
        ++tooDark;
    }
  }

  TEST_GREATER(teal, 0u, ("Pattern not visible:", title));
  TEST_LESS(teal, kW * kH / 2, ("No gaps - pattern degenerated into a solid fill?", title));
  TEST_EQUAL(tooDark, 0u, ("Fill darker than a straight-alpha blend - is rgb modulated too?", title));
}

// A solid-fill pattern (stipple/speckle/grid) fills a quad with the surface colour and modulates it with dots that
// darken a light fill and lighten a dark one. Renders a light quad in the left half and a dark one in the right half,
// and validates that both fills are present, their dots shade them the expected way, and nothing samples as black.
void RenderSolidPatternAndCheck(char const * title, std::string_view patternKey)
{
  df::test_support::ShapeTestFixture fixture;
  uint32_t constexpr kW = 256, kH = 256;
  fixture.Render(title, kW, kH, [patternKey](df::test_support::ShapeTestFixture & f)
  {
    for (auto const & [color, x] : {std::pair{kLightFill, -120.0}, std::pair{kDarkFill, 8.0}})
    {
      df::AreaViewParams p = MakeParams(patternKey);
      p.m_color = color;
      std::vector<m2::PointD> triangles = {{x, -110}, {x + 112, -110}, {x + 112, 110},
                                           {x, -110}, {x + 112, 110},  {x, 110}};
      f.AddShape(make_unique_dp<df::AreaShape>(std::move(triangles), df::BuildingOutline{}, p));
    }
  });

  QImage const & img = fixture.GetLastImage();
  if (img.isNull())
    return;  // Headless env without a usable GL context - nothing to assert.

  uint32_t fill[2] = {}, dots[2] = {}, opaqueBlack = 0;
  for (int y = 0; y < img.height(); ++y)
  {
    for (int x = 0; x < img.width(); ++x)
    {
      QColor const c = img.pixelColor(x, y);
      size_t const i = x < img.width() / 2 ? 0 : 1;
      if (c.green() > c.red() + 20 && c.blue() > c.red() + 20)  // teal fill on the pattern
      {
        ++fill[i];
        int const fillGreen = (i == 0 ? kLightFill : kDarkFill).GetGreen();
        if (i == 0 ? c.green() < fillGreen - 4 : c.green() > fillGreen + 15)
          ++dots[i];
      }
      if (c.alpha() > 200 && c.red() < 8 && c.green() < 8 && c.blue() < 8)
        ++opaqueBlack;
    }
  }

  for (size_t i = 0; i < 2; ++i)
  {
    char const * fillName = i == 0 ? "light fill" : "dark fill";
    TEST_GREATER(fill[i], kW * kH / 8, ("Solid fill not rendered:", title, fillName));
    TEST_GREATER(dots[i], 0u, ("Dots not visible or shading the wrong way:", title, fillName));
  }
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
