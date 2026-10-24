#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "coding/point_coding.hpp"

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
  using Framework::OnViewportChanged;

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

UNIT_TEST(Framework_GetVisibleViewport)
{
  TestFramework framework;
  ScreenBase screen;
  screen.OnSize(0, 0, 1000, 800);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(9.0, 19.2, 11.0, 20.8)));
  framework.SetViewport(screen, screen.PixelRectIn3d());
  TEST_EQUAL(framework.GetVisibleViewport(), framework.GetCurrentViewport(), ());

  framework.SetViewport(screen, m2::RectD(300, 0, 1000, 800));
  auto const visible = framework.GetVisibleViewport();
  TEST_ALMOST_EQUAL_ABS(visible.minX(), 9.6, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(visible.maxX(), 11.0, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(visible.minY(), 19.2, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(visible.maxY(), 20.8, 1e-9, ());
  auto const covered = screen.PtoG(m2::PointD(100, 400));
  TEST(framework.GetCurrentViewport().IsPointInside(covered), ());
  TEST(!visible.IsPointInside(covered), ());
}

UNIT_TEST(Framework_GetVisibleViewportWithRotationAndPerspective)
{
  TestFramework framework;
  for (double const angle : {0.0, 0.7})
  {
    for (bool const perspective : {false, true})
    {
      ScreenBase screen;
      screen.OnSize(0, 0, 1000, 800);
      screen.SetFromRect(m2::AnyRectD(m2::PointD(10, 20), ang::AngleD(angle), m2::RectD(-1, -0.8, 1, 0.8)));
      if (perspective)
        screen.ApplyPerspective(math::pi4, math::pi4, math::pi / 3.0);
      m2::RectD const viewport(300, 100, 900, 700);
      framework.SetViewport(screen, viewport);
      auto const visible = framework.GetVisibleViewport();
      auto padded = visible;
      padded.Inflate(1e-9, 1e-9);
      for (int x = 0; x <= 4; ++x)
      {
        for (int y = 0; y <= 4; ++y)
        {
          m2::PointD const pixel(viewport.minX() + viewport.SizeX() * x / 4,
                                 viewport.minY() + viewport.SizeY() * y / 4);
          TEST(padded.IsPointInside(screen.PtoG(screen.P3dtoP(pixel))), (angle, perspective, pixel));
        }
      }
      auto const full = framework.GetCurrentViewport();
      TEST_LESS(visible.SizeX() * visible.SizeY(), full.SizeX() * full.SizeY(), (angle, perspective));
    }
  }
}

UNIT_TEST(Framework_VisibleViewportChangesWithRotation)
{
  TestFramework framework;
  ScreenBase screen;
  screen.OnSize(0, 0, 1000, 1000);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(9.0, 19.0, 11.0, 21.0)));
  m2::RectD const viewport(300, 0, 1000, 1000);

  for (double const angle : {math::pi / 2.0, math::pi})
  {
    framework.SetViewport(screen, viewport);
    auto rotated = screen;
    rotated.SetAngle(angle);
    TEST(m2::IsEqual(rotated.ClipRect(), screen.ClipRect(), 1e-9, 1e-9), ());

    framework.OnViewportChanged(rotated);
    auto const visible = framework.GetVisibleViewport();
    auto const covered = rotated.PtoG(rotated.P3dtoP(m2::PointD(100, 500)));
    auto const exposed = rotated.PtoG(rotated.P3dtoP(m2::PointD(900, 500)));
    TEST(!visible.IsPointInside(covered), (angle, visible, covered));
    TEST(visible.IsPointInside(exposed), (angle, visible, exposed));
  }
}

UNIT_TEST(Framework_ViewportUpdatesAccumulateSmallMovements)
{
  TestFramework framework;
  ScreenBase screen;
  screen.OnSize(0, 0, 1000, 1000);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(9.0, 19.0, 11.0, 21.0)));
  framework.SetViewport(screen, m2::RectD(300, 0, 1000, 1000));
  int updates = 0;
  framework.SetViewportListener([&](ScreenBase const &) { ++updates; });

  for (int i = 1; i <= 8; ++i)
  {
    auto moved = screen;
    moved.MoveG(m2::PointD(kMwmPointAccuracy * i / 4.0, 0.0));
    framework.OnViewportChanged(moved);
  }
  TEST_GREATER(updates, 0, ());
  TEST_LESS(updates, 8, ());
}
}  // namespace framework_viewport_tests
