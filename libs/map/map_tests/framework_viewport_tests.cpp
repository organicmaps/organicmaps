#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/mercator.hpp"

#include "base/math.hpp"

namespace framework_viewport_tests
{
namespace
{
class TestFramework final : public Framework
{
public:
  TestFramework() : Framework({}, false /* loadMaps */) {}

  void SetViewport(ScreenBase const & screen, m2::RectD const & visibleViewport)
  {
    m_currentModelView = screen;
    m_visibleViewport = visibleViewport;
  }
};
}  // namespace

UNIT_TEST(Framework_GetViewportCenter)
{
  m2::RectD const pixelRect(0.0, 0.0, 1000.0, 1000.0);
  m2::RectD const visibleViewport(0.0, 0.0, 1000.0, 700.0);
  m2::PointD const target(185.0, 5.0);
  double constexpr kEps = 1e-7;

  TestFramework framework;
  for (bool const perspective : {false, true})
  {
    ScreenBase screen;
    screen.SetFromRects(m2::AnyRectD(m2::RectD(170.0, -10.0, 190.0, 10.0)), pixelRect);
    if (perspective)
      screen.ApplyPerspective(math::pi4, math::pi4, math::pi / 3.0);
    screen.MatchGandP3d(target, visibleViewport.Center());

    framework.SetViewport(screen, visibleViewport);
    auto const actual = framework.GetViewportCenter();
    TEST_ALMOST_EQUAL_ABS(actual.x, mercator::WrapX(target.x), kEps, (perspective));
    TEST_ALMOST_EQUAL_ABS(actual.y, target.y, kEps, (perspective));
  }
}

UNIT_TEST(Framework_GtoP3d)
{
  m2::RectD const pixelRect(0.0, 0.0, 1000.0, 1000.0);
  // East of the antimeridian, where P3dtoG wraps the point to the other world copy.
  m2::PointD const pixel(800.0, 300.0);
  double constexpr kEps = 1e-3;

  TestFramework framework;
  for (bool const perspective : {false, true})
  {
    ScreenBase screen;
    screen.SetFromRects(m2::AnyRectD(m2::RectD(170.0, -10.0, 190.0, 10.0)), pixelRect);
    if (perspective)
      screen.ApplyPerspective(math::pi4, math::pi4, math::pi / 3.0);

    framework.SetViewport(screen, pixelRect);
    auto const geo = framework.P3dtoG(pixel);
    TEST_LESS(geo.x, 0.0, (perspective));
    auto const actual = framework.GtoP3d(geo);
    TEST_ALMOST_EQUAL_ABS(actual.x, pixel.x, kEps, (perspective));
    TEST_ALMOST_EQUAL_ABS(actual.y, pixel.y, kEps, (perspective));
  }
}
}  // namespace framework_viewport_tests
