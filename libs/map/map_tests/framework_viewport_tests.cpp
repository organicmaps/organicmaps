#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <vector>

namespace framework_viewport_tests
{
namespace
{
class TestFramework final : public Framework
{
public:
  TestFramework() : Framework({}, false /* loadMaps */) {}

  using Framework::OnViewportChanged;

  void SetViewport(ScreenBase const & screen, m2::RectD const & visibleViewport)
  {
    m_currentModelView = screen;
    m_visibleViewport = visibleViewport;
  }
};

// Returns how many of the |screens|, reported by the drape engine in a row, reach the viewport listener.
size_t CountViewportChanges(std::vector<ScreenBase> const & screens)
{
  size_t count = 0;
  TestFramework framework;
  framework.SetViewportListener([&count](ScreenBase const &) { ++count; });
  for (auto const & screen : screens)
    framework.OnViewportChanged(screen);
  return count;
}

// A pixel is ten times shorter than kMwmPointAccuracy.
ScreenBase GetHighZoomScreen()
{
  ScreenBase screen;
  screen.SetFromParams(m2::PointD(10.0, 10.0), 0.0 /* angle */, 1e-6 /* scale */);
  return screen;
}

// Converts |pixel| to a point on |screen|, reports |changed| as the next viewport and returns the new pixel of the
// point, after checking that the reverse conversion agrees.
m2::PointD GetPixelAfterChange(ScreenBase const & screen, ScreenBase const & changed, m2::PointD const & pixel)
{
  TestFramework framework;
  framework.OnViewportChanged(screen);
  auto const geo = framework.P3dtoG(pixel);
  framework.OnViewportChanged(changed);
  auto const actual = framework.GtoP3d(geo);
  TEST(actual, ());
  TEST(m2::AlmostEqualAbs(framework.P3dtoG(*actual), geo, 1e-12), ());
  return *actual;
}

ScreenBase GetSquareHighZoomScreen()
{
  auto screen = GetHighZoomScreen();
  screen.OnSize(0, 0, 1000, 1000);
  return screen;
}
}  // namespace

UNIT_TEST(Framework_ViewportListener_SameScreen)
{
  auto const screen = GetHighZoomScreen();
  auto moved = screen;
  moved.Move(0.25 /* dx */, 0.0 /* dy */);
  TEST_EQUAL(CountViewportChanges({screen, screen, moved}), 1, ());
}

UNIT_TEST(Framework_ViewportListener_OnePixelPan)
{
  auto const screen = GetHighZoomScreen();
  auto moved = screen;
  moved.Move(1.0 /* dx */, 0.0 /* dy */);
  TEST_EQUAL(CountViewportChanges({screen, moved}), 2, ());
}

UNIT_TEST(Framework_ViewportListener_HalfTurn)
{
  auto const screen = GetHighZoomScreen();
  auto rotated = screen;
  rotated.Rotate(math::pi);
  TEST_EQUAL(CountViewportChanges({screen, rotated}), 2, ());
}

UNIT_TEST(Framework_GtoP3d_AfterSmallPan)
{
  auto const screen = GetSquareHighZoomScreen();
  auto moved = screen;
  moved.Move(5.0 /* dx */, 0.0 /* dy */);
  auto const actual = GetPixelAfterChange(screen, moved, m2::PointD(700.0, 300.0));
  TEST(m2::AlmostEqualAbs(actual, m2::PointD(705.0, 300.0), 1e-6), (actual));
}

UNIT_TEST(Framework_GtoP3d_AfterHalfTurn)
{
  auto const screen = GetSquareHighZoomScreen();
  auto rotated = screen;
  rotated.Rotate(math::pi);
  auto const actual = GetPixelAfterChange(screen, rotated, m2::PointD(700.0, 300.0));
  TEST(m2::AlmostEqualAbs(actual, m2::PointD(300.0, 700.0), 1e-6), (actual));
}

UNIT_TEST(Framework_IsViewportInitialized)
{
  TestFramework framework;
  TEST(!framework.IsViewportInitialized(), ());
  framework.OnViewportChanged(GetHighZoomScreen());
  TEST(framework.IsViewportInitialized(), ());
}

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
    screen.OnSize(m2::RectI(pixelRect));
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
    screen.OnSize(m2::RectI(pixelRect));
    screen.SetFromRects(m2::AnyRectD(m2::RectD(170.0, -10.0, 190.0, 10.0)), pixelRect);
    if (perspective)
      screen.ApplyPerspective(math::pi4, math::pi4, math::pi / 3.0);

    TEST(screen.PixelRectIn3d().IsPointInside(pixel), (perspective));

    framework.SetViewport(screen, pixelRect);
    auto const geo = framework.P3dtoG(pixel);
    TEST_LESS(geo.x, 0.0, (perspective));
    auto const actual = framework.GtoP3d(geo);
    TEST(actual, (perspective));
    TEST_ALMOST_EQUAL_ABS(actual->x, pixel.x, kEps, (perspective));
    TEST_ALMOST_EQUAL_ABS(actual->y, pixel.y, kEps, (perspective));
  }
}

UNIT_TEST(Framework_GtoP3d_BehindCamera)
{
  m2::RectD const pixelRect(0.0, 0.0, 1000.0, 1000.0);
  // South of the camera, which in 3D sits south of the screen center and looks north.
  m2::PointD const behind = mercator::FromLatLon(41.89, 12.5);
  double constexpr kEps = 1e-3;

  TestFramework framework;
  for (bool const perspective : {false, true})
  {
    ScreenBase screen;
    screen.OnSize(m2::RectI(pixelRect));
    // The automatic perspective tilts the map by 45 degrees at this scale.
    screen.SetFromParams(mercator::FromLatLon(41.9, 12.5), 0.0 /* angle */, 3e-6 /* scale */);
    screen.SetAutoPerspective(perspective);
    TEST_EQUAL(screen.isPerspective(), perspective, ());
    framework.SetViewport(screen, pixelRect);

    // Off the screen but in front of the camera, beyond each edge.
    for (m2::PointD const pixel :
         {m2::PointD(500.0, -300.0), m2::PointD(1300.0, 500.0), m2::PointD(-300.0, 500.0), m2::PointD(500.0, 2000.0)})
    {
      auto const geo = framework.TryP3dtoG(pixel);
      TEST(geo, (perspective, pixel));
      auto const actual = framework.GtoP3d(*geo);
      TEST(actual, (perspective, pixel));
      TEST_ALMOST_EQUAL_ABS(actual->x, pixel.x, kEps, (perspective, pixel));
      TEST_ALMOST_EQUAL_ABS(actual->y, pixel.y, kEps, (perspective, pixel));
    }

    auto const actual = framework.GtoP3d(behind);
    if (perspective)
    {
      TEST(!actual, (*actual));
    }
    else
    {
      TEST(actual, ());
      TEST_GREATER(actual->y, pixelRect.maxY(), ());
    }
  }
}

UNIT_TEST(Framework_TryP3dtoG_AboveHorizon)
{
  m2::RectD const pixelRect(0.0, 0.0, 1000.0, 1000.0);
  double constexpr kEps = 1e-3;
  // In 3D the horizon is above the view: about 366 pixels above at a 45 degree tilt, and 115 at 54 degrees.
  struct
  {
    double m_scale;
    m2::PointD m_belowHorizon;
    m2::PointD m_aboveHorizon;
  } const cases[] = {{3e-6, {500.0, -300.0}, {500.0, -400.0}}, {1e-6, {500.0, -100.0}, {500.0, -200.0}}};

  TestFramework framework;
  for (auto const & c : cases)
  {
    for (bool const perspective : {false, true})
    {
      ScreenBase screen;
      screen.OnSize(m2::RectI(pixelRect));
      screen.SetFromParams(mercator::FromLatLon(41.9, 12.5), 0.0 /* angle */, c.m_scale);
      screen.SetAutoPerspective(perspective);
      TEST_EQUAL(screen.isPerspective(), perspective, ());
      framework.SetViewport(screen, pixelRect);

      for (m2::PointD const pixel : {pixelRect.LeftTop(), pixelRect.RightTop(), pixelRect.LeftBottom(),
                                     pixelRect.RightBottom(), c.m_belowHorizon})
      {
        auto const geo = framework.TryP3dtoG(pixel);
        TEST(geo, (c.m_scale, perspective, pixel));
        auto const actual = framework.GtoP3d(*geo);
        TEST(actual, (c.m_scale, perspective, pixel));
        TEST(m2::AlmostEqualAbs(*actual, pixel, kEps), (c.m_scale, perspective, pixel, *actual));
      }

      auto const geo = framework.TryP3dtoG(c.m_aboveHorizon);
      TEST_EQUAL(geo.has_value(), !perspective, (c.m_scale, mercator::ToLatLon(framework.P3dtoG(c.m_aboveHorizon))));
    }
  }
}
}  // namespace framework_viewport_tests
