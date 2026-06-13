#include "testing/testing.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"

#include "drape/color.hpp"
#include "drape/hatching_decl.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace hatching_area_gpu_test
{
df::AreaViewParams MakeHatchParams(double baseGtoPScale)
{
  df::AreaViewParams p;
  p.m_tileCenter = {0, 0};
  p.m_color = dp::Color(0, 160, 160, 255);  // teal, clearly distinct from the white background
  p.m_depth = 0;
  p.m_depthTestEnabled = false;
  p.m_depthLayer = df::DepthLayer::GeometryLayer;
  p.m_minVisibleScale = 0;
  p.m_rank = 0;
  p.m_hatching = dp::kDashHatching;  // mipmapped mask (see TextureManager init)
  p.m_baseGtoPScale = baseGtoPScale;
  return p;
}
}  // namespace hatching_area_gpu_test

// Renders a wetland-style dash-hatched area through the real GL pipeline with the mask minified
// (baseGtoPScale > 1 => the 16px pattern is squeezed below 1 texel/pixel), so a generated mip level is
// actually sampled. Asserts the hatch colour shows up: an incomplete mip chain would sample black and an
// unbound/empty mask would leave the frame white - both would fail this.
UNIT_TEST(HatchingAreaGpuTest)
{
  using namespace hatching_area_gpu_test;

  df::test_support::ShapeTestFixture fixture;
  uint32_t constexpr kW = 256, kH = 256;
  fixture.Render("Hatching area (mipmapped mask)", kW, kH, [](df::test_support::ShapeTestFixture & f)
  {
    // Quad covering most of the viewport (two triangles), in world == pixel-from-center coords.
    std::vector<m2::PointD> triangles = {{-110, -110}, {110, -110}, {110, 110}, {-110, -110}, {110, 110}, {-110, 110}};
    f.AddShape(make_unique_dp<df::AreaShape>(std::move(triangles), df::BuildingOutline{},
                                             MakeHatchParams(2.0 /* baseGtoPScale */)));
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
      // Hatch colour: green & blue clearly above red (teal over white => G,B > R).
      if (c.green() > c.red() + 20 && c.blue() > c.red() + 20)
        ++teal;
      // An incomplete mip chain samples as opaque black (0,0,0,1). Transparent dash gaps are (0,0,0,0)
      // and must not count - only opaque near-black pixels indicate the failure.
      if (c.alpha() > 200 && c.red() < 8 && c.green() < 8 && c.blue() < 8)
        ++opaqueBlack;
    }
  }

  TEST_GREATER(teal, 0u, ("Hatch colour not visible - mask not sampled?"));
  TEST_EQUAL(opaqueBlack, 0u, ("Opaque black pixels indicate an incomplete mipmap chain"));
}
