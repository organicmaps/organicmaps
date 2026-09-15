#include "testing/testing.hpp"

#include "drape_frontend/area_pattern.hpp"
#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"

#include "drape/color.hpp"

#include "geometry/point2d.hpp"

#include <cstdlib>
#include <utility>
#include <vector>

namespace area_pattern_gpu_test
{
// Teal-ish fills with a luma far above and below the 0.5 split between darker and lighter pattern dots. The dark one
// is near-black, where a dot has to lighten the fill by more than a multiplier could.
dp::Color constexpr kLightFill(196, 233, 239, 255);
dp::Color constexpr kDarkFill(2, 25, 25, 255);

// Two solid-pattern quads side by side, by the x of their left edge in world == pixel-from-center coordinates.
int constexpr kQuadWidth = 112, kQuadHalfHeight = 110;
std::pair<dp::Color, int> constexpr kQuads[] = {{kLightFill, -120}, {kDarkFill, 8}};

df::AreaViewParams MakeParams(df::AreaPattern pattern)
{
  df::AreaViewParams p;
  p.m_tileCenter = {0, 0};
  p.m_color = dp::Color(0, 160, 160, 255);  // teal, clearly distinct from the background
  p.m_depth = 0;
  p.m_depthTestEnabled = false;
  p.m_depthLayer = df::DepthLayer::GeometryLayer;
  p.m_minVisibleScale = 0;
  p.m_rank = 0;
  p.m_areaPattern = pattern;
  p.m_baseGtoPScale = 1.0;  // pattern at base scale: crisp 1px features with clear gaps
  return p;
}

// Renders a hatched quad through the real GL pipeline and asserts the analytic pattern behaves: the fill
// shows up on covered fragments, the quad is NOT flooded (gaps exist => it is a pattern, not a solid
// fill), and every pixel lands on the straight-alpha blend of the fill over the background. No mask
// texture is bound, so a binding mistake, a broken coverage function or coverage applied to rgb as well
// would surface here as an empty frame, a flooded quad, or a fill darker than the blend allows.
void RenderAndCheck(char const * title, df::AreaPattern hatching)
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
  // coverage is. Scaling rgb by coverage as well (or sampling an unbound color texture) can only drag g
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

// A solid-fill pattern fills a quad with the surface color and modulates it with marks that darken a light fill and
// lighten a dark one. Renders the light and the dark quad and validates that every pixel of each lies between the fill
// and its marked color. A transparent fragment would let the white background through, which the blending keeps at
// alpha 255, so opacity is checked on the color. At the base scale each quad shows both its plain fill and marks. With
// |faded|, the pattern is minified past the forest's fade at 4 lattice px per screen px, and each quad has to be one
// even tint of the fill without marks.
void RenderSolidPatternAndCheck(char const * title, df::AreaPattern pattern, bool faded = false)
{
  df::test_support::ShapeTestFixture fixture;
  int constexpr kW = 256, kH = 256;
  fixture.Render(title, kW, kH, [pattern, faded](df::test_support::ShapeTestFixture & f)
  {
    for (auto const & [color, quadX] : kQuads)
    {
      df::AreaViewParams p = MakeParams(pattern);
      p.m_color = color;
      p.m_baseGtoPScale = faded ? 5.0 : 1.0;
      double const x = quadX, right = x + kQuadWidth;
      std::vector<m2::PointD> triangles = {{x, -kQuadHalfHeight}, {right, -kQuadHalfHeight}, {right, kQuadHalfHeight},
                                           {x, -kQuadHalfHeight}, {right, kQuadHalfHeight},  {x, kQuadHalfHeight}};
      f.AddShape(make_unique_dp<df::AreaShape>(std::move(triangles), df::BuildingOutline{}, p));
    }
  });

  QImage const & img = fixture.GetLastImage();
  if (img.isNull())
    return;  // Headless env without a usable GL context - nothing to assert.

  // The quads' pixels without their border.
  int const top = kH / 2 - kQuadHalfHeight + 1, bottom = kH / 2 + kQuadHalfHeight - 1;
  for (auto const & [fill, quadX] : kQuads)
  {
    int const left = kW / 2 + quadX + 1, right = kW / 2 + quadX + kQuadWidth - 1;
    bool const isLight = fill == kLightFill;
    QColor const first = img.pixelColor(left, top);
    uint32_t plain = 0, marked = 0, offFill = 0, uneven = 0;
    for (int y = top; y < bottom; ++y)
    {
      for (int x = left; x < right; ++x)
      {
        QColor const c = img.pixelColor(x, y);
        int const dr = c.red() - fill.GetRed(), dg = c.green() - fill.GetGreen(), db = c.blue() - fill.GetBlue();
        if (std::abs(dr) <= 1 && std::abs(dg) <= 1 && std::abs(db) <= 1)
          ++plain;
        else if (isLight ? dg < -4 : dg > 4)
          ++marked;
        // A mark scales a light fill down and adds the same offset to every channel of a dark one.
        bool const onFill = isLight ? dr <= 1 && dg <= 1 && db <= 1 && c.green() >= fill.GetGreen() * 3 / 4
                                    : std::abs(dr - dg) <= 2 && std::abs(db - dg) <= 2 && dg >= -1 && dg <= 20;
        if (!onFill)
          ++offFill;
        if (c != first)
          ++uneven;
      }
    }

    char const * fillName = isLight ? "light fill" : "dark fill";
    TEST_EQUAL(offFill, 0u, ("Pixels off the fill - transparent, or color texture not sampled?", title, fillName));
    if (faded)
    {
      // A dropped fade leaves the plain fill, since the shader then skips the crowns, so the tint direction is checked.
      int const dg = first.green() - fill.GetGreen();
      TEST_EQUAL(uneven, 0u, ("Minified marks don't fade into an even tint:", title, fillName));
      TEST_EQUAL(marked, 0u, ("Minified pattern still marked:", title, fillName));
      TEST(isLight ? dg < 0 : dg > 0, ("Minified pattern doesn't tint the fill:", title, fillName, dg));
    }
    else
    {
      TEST_GREATER(plain, 0u, ("No plain fill between the marks:", title, fillName));
      TEST_GREATER(marked, 0u, ("Marks not visible or shading the wrong way:", title, fillName));
    }
  }
}
}  // namespace area_pattern_gpu_test

UNIT_TEST(AreaHatch45GpuTest)
{
  area_pattern_gpu_test::RenderAndCheck("Analytic 45d hatch", df::AreaPattern::Hatch45d);
}

UNIT_TEST(AreaHatchDashGpuTest)
{
  area_pattern_gpu_test::RenderAndCheck("Analytic dash hatch", df::AreaPattern::HatchDash);
}

UNIT_TEST(AreaStippleGpuTest)
{
  area_pattern_gpu_test::RenderSolidPatternAndCheck("Analytic stipple", df::AreaPattern::Stipple);
}

UNIT_TEST(AreaSpeckleGpuTest)
{
  area_pattern_gpu_test::RenderSolidPatternAndCheck("Analytic speckle", df::AreaPattern::Speckle);
}

UNIT_TEST(AreaGridGpuTest)
{
  area_pattern_gpu_test::RenderSolidPatternAndCheck("Analytic grid", df::AreaPattern::Grid);
}

UNIT_TEST(AreaForestGpuTest)
{
  area_pattern_gpu_test::RenderSolidPatternAndCheck("Analytic forest", df::AreaPattern::Forest);
}

UNIT_TEST(AreaForestFadeGpuTest)
{
  area_pattern_gpu_test::RenderSolidPatternAndCheck("Faded analytic forest", df::AreaPattern::Forest, true /* faded */);
}
