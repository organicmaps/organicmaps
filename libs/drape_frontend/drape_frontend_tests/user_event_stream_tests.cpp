#include "testing/testing.hpp"

#include "drape_frontend/animation_constants.hpp"
#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "indexer/scales.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/mercator.hpp"

#include "base/scope_guard.hpp"
#include "base/thread.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <list>

namespace user_event_stream_test
{
using df::test_support::VisualParamsFixture;
using namespace std::placeholders;

namespace
{
class UserEventStreamTest
{
public:
  UserEventStreamTest()
  {
#ifdef DEBUG
    m_stream.SetTestBridge(std::bind(&UserEventStreamTest::TestBridge, this, _1));
#endif
  }

  void AddUserEvent(df::TouchEvent const & event) { m_stream.AddEvent(make_unique_dp<df::TouchEvent>(event)); }

  void SetRect(m2::RectD const & r)
  {
    m_stream.AddEvent(make_unique_dp<df::SetRectEvent>(
        r, false /* rotate */, false /* isAnim */, false /* useVisibleViewport */, nullptr /* parallelAnimCreator */));
  }

  void ShowRect(m2::RectD const & r, bool isAnim)
  {
    m_stream.AddEvent(make_unique_dp<df::SetRectEvent>(r, false /* rotate */, isAnim, true /* useVisibleViewport */,
                                                       nullptr /* parallelAnimCreator */));
  }

  void SetAnyRect(m2::AnyRectD const & r)
  {
    m_stream.AddEvent(make_unique_dp<df::SetAnyRectEvent>(r, false /* isAnim */, false /* fitInViewport */,
                                                          false /* useVisibleViewport */));
  }

  void SetAutoPerspective(bool isAutoPerspective)
  {
    m_stream.AddEvent(make_unique_dp<df::SetAutoPerspectiveEvent>(isAutoPerspective));
  }

  // Advances the animations frame by frame, as the renderer does.
  void FinishAnimations()
  {
    double constexpr kFrameSec = 1.0 / 60.0;
    int constexpr kMaxFrames = 60 * 10;
    auto & animationSystem = df::AnimationSystem::Instance();
    for (int i = 0; i < kMaxFrames && animationSystem.HasAnimations(); ++i)
    {
      animationSystem.Advance(kFrameSec);
      RunTest();
    }
    TEST(!animationSystem.HasAnimations(), ());
  }

  void AddResizeEvent(uint32_t w, uint32_t h) { m_stream.AddEvent(make_unique_dp<df::ResizeEvent>(w, h)); }

  void AddSetVisibleViewport(m2::RectD const & rect)
  {
    m_stream.AddEvent(make_unique_dp<df::SetVisibleViewportEvent>(rect));
  }

  void AddSetCenter(m2::PointD const & center, int zoom, bool trackVisibleViewport)
  {
    m_stream.AddEvent(make_unique_dp<df::SetCenterEvent>(center, zoom, false /* isAnim */, trackVisibleViewport,
                                                         nullptr /* parallelAnimCreator */));
  }

  ScreenBase const & GetScreen() const { return m_stream.GetCurrentScreen(); }

  void AddExpectation(char const * action) { m_expectation.push_back(action); }

  void RunTest()
  {
    bool dummy1, dummy2, dummy3;
    m_stream.ProcessEvents(dummy1, dummy2, dummy3);
    TEST_EQUAL(m_expectation.empty(), true, ());
  }

private:
#ifdef DEBUG
  void TestBridge(char const * action)
  {
    TEST(!m_expectation.empty(), ());
    char const * a = m_expectation.front();
    TEST_EQUAL(strcmp(action, a), 0, ());
    m_expectation.pop_front();
  }
#endif

private:
  df::UserEventStream m_stream;
  std::list<char const *> m_expectation;
};

#ifdef DEBUG
int touchTimeStamp = 1;

df::TouchEvent MakeTouchEvent(m2::PointD const & pt1, m2::PointD const & pt2, df::TouchEvent::ETouchType type)
{
  df::TouchEvent e;
  df::Touch t1;
  t1.m_location = pt1;
  t1.m_id = 1;
  e.SetFirstTouch(t1);

  df::Touch t2;
  t2.m_location = pt2;
  t2.m_id = 2;
  e.SetSecondTouch(t2);

  e.SetTouchType(type);
  e.SetFirstMaskedPointer(0);
  e.SetSecondMaskedPointer(1);
  e.SetTimeStamp(touchTimeStamp++);

  return e;
}

df::TouchEvent MakeTouchEvent(m2::PointD const & pt, df::TouchEvent::ETouchType type)
{
  df::TouchEvent e;

  df::Touch t1;
  t1.m_location = pt;
  t1.m_id = 1;
  e.SetFirstTouch(t1);

  e.SetTouchType(type);
  e.SetFirstMaskedPointer(0);
  e.SetTimeStamp(touchTimeStamp++);

  return e;
}
#endif

/// Checks that the corners of the global rect are drawn inside the pixel rect.
void TestRectIsVisible(ScreenBase const & screen, m2::RectD const & rect, m2::RectD const & pixelRect)
{
  double constexpr kEps = 1.0;
  m2::RectD inflated = pixelRect;
  inflated.Inflate(kEps, kEps);
  for (auto const & corner : {rect.LeftBottom(), rect.LeftTop(), rect.RightTop(), rect.RightBottom()})
  {
    m2::PointD const pt = screen.PtoP3d(screen.GtoP(corner));
    TEST(inflated.IsPointInside(pt), (pt, "outside", pixelRect));
  }
}

/// Checks that the rect is not drawn much smaller than it could be: a corner is close to an edge of the pixel rect.
void TestRectFills(ScreenBase const & screen, m2::RectD const & rect, m2::RectD const & pixelRect)
{
  double ratio = 0.0;
  for (auto const & corner : {rect.LeftBottom(), rect.LeftTop(), rect.RightTop(), rect.RightBottom()})
  {
    m2::PointD const offset = screen.PtoP3d(screen.GtoP(corner)) - pixelRect.Center();
    ratio =
        std::max({ratio, std::fabs(offset.x) * 2.0 / pixelRect.SizeX(), std::fabs(offset.y) * 2.0 / pixelRect.SizeY()});
  }
  TEST_GREATER(ratio, 0.99, ());
}

void TestPointIsAt(ScreenBase const & screen, m2::PointD const & g, m2::PointD const & expected)
{
  double constexpr kEps = 1.0;
  m2::PointD const pt = screen.PtoP3d(screen.GtoP(g));
  TEST(pt.EqualDxDy(expected, kEps), (pt, "expected", expected));
}

m2::RectD RectFromLatLon(double minLat, double minLon, double maxLat, double maxLon)
{
  return m2::RectD(mercator::FromLatLon(minLat, minLon), mercator::FromLatLon(maxLat, maxLon));
}
}  // namespace

// The gestures are reported to the test bridge in Debug only.
#ifdef DEBUG
UNIT_CLASS_TEST(VisualParamsFixture, SimpleTap)
{
  UserEventStreamTest test;
  test.AddExpectation(df::UserEventStream::TRY_FILTER);
  test.AddExpectation(df::UserEventStream::BEGIN_TAP_DETECTOR);
  test.AddExpectation(df::UserEventStream::SHORT_TAP_DETECTED);

  test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_DOWN));
  test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_UP));

  test.RunTest();
}

UNIT_CLASS_TEST(VisualParamsFixture, SimpleLongTap)
{
  UserEventStreamTest test;
  test.AddExpectation(df::UserEventStream::TRY_FILTER);
  test.AddExpectation(df::UserEventStream::BEGIN_TAP_DETECTOR);
  test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_DOWN));
  test.RunTest();

  threads::Sleep(1100);
  test.AddExpectation(df::UserEventStream::LONG_TAP_DETECTED);
  test.RunTest();

  test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_UP));
  test.RunTest();
}

UNIT_CLASS_TEST(VisualParamsFixture, SimpleDrag)
{
  size_t const moveEventCount = 5;
  UserEventStreamTest test;
  test.AddExpectation(df::UserEventStream::TRY_FILTER);
  test.AddExpectation(df::UserEventStream::BEGIN_TAP_DETECTOR);
  test.AddExpectation(df::UserEventStream::CANCEL_TAP_DETECTOR);
  test.AddExpectation(df::UserEventStream::BEGIN_DRAG);
  for (size_t i = 0; i < moveEventCount - 2; ++i)
    test.AddExpectation(df::UserEventStream::DRAG);
  test.AddExpectation(df::UserEventStream::END_DRAG);

  m2::PointD pointer = m2::PointD::Zero();
  test.AddUserEvent(MakeTouchEvent(pointer, df::TouchEvent::TOUCH_DOWN));
  for (size_t i = 0; i < 5; ++i)
  {
    pointer += m2::PointD(100.0, 0.0);
    test.AddUserEvent(MakeTouchEvent(pointer, df::TouchEvent::TOUCH_MOVE));
  }
  test.AddUserEvent(MakeTouchEvent(pointer, df::TouchEvent::TOUCH_UP));
  test.RunTest();
}

UNIT_CLASS_TEST(VisualParamsFixture, SimpleScale)
{
  size_t const moveEventCount = 5;
  UserEventStreamTest test;
  test.SetRect(m2::RectD(-250.0, -250.0, 250.0, 250.0));

  test.AddExpectation(df::UserEventStream::TWO_FINGERS_TAP);
  test.AddExpectation(df::UserEventStream::BEGIN_SCALE);
  for (size_t i = 0; i < moveEventCount - 1; ++i)
    test.AddExpectation(df::UserEventStream::SCALE);
  test.AddExpectation(df::UserEventStream::END_SCALE);

  m2::PointD pointer1 = m2::PointD::Zero() + m2::PointD(10.0, 10.0);
  m2::PointD pointer2 = m2::PointD::Zero() - m2::PointD(10.0, 10.0);
  test.AddUserEvent(MakeTouchEvent(pointer1, pointer2, df::TouchEvent::TOUCH_DOWN));
  for (size_t i = 0; i < moveEventCount; ++i)
  {
    pointer1 += m2::PointD(20.0, 0.0);
    pointer2 -= m2::PointD(20.0, 0.0);
    test.AddUserEvent(MakeTouchEvent(pointer1, pointer2, df::TouchEvent::TOUCH_MOVE));
  }
  test.AddUserEvent(MakeTouchEvent(pointer1, pointer2, df::TouchEvent::TOUCH_UP));
  test.RunTest();
}

UNIT_CLASS_TEST(VisualParamsFixture, DragThresholdFollowsVisualScale)
{
  SCOPE_GUARD(restoreScale, []() { df::VisualParams::Instance().SetVisualScale(1.0); });
  for (auto const & scales : {std::pair{1.0, 2.0}, std::pair{2.0, 1.0}})
  {
    df::VisualParams::Instance().SetVisualScale(scales.first);
    UserEventStreamTest test;
    test.AddExpectation(df::UserEventStream::TRY_FILTER);
    test.AddExpectation(df::UserEventStream::BEGIN_TAP_DETECTOR);
    test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_DOWN));
    test.RunTest();

    df::VisualParams::Instance().SetVisualScale(scales.second);
    double const threshold = 10.0 * scales.second;
    test.AddUserEvent(MakeTouchEvent(m2::PointD(threshold - 1, 0), df::TouchEvent::TOUCH_MOVE));
    test.AddUserEvent(MakeTouchEvent(m2::PointD(threshold, 0), df::TouchEvent::TOUCH_MOVE));
    test.RunTest();

    test.AddExpectation(df::UserEventStream::CANCEL_TAP_DETECTOR);
    test.AddUserEvent(MakeTouchEvent(m2::PointD(threshold + 1, 0), df::TouchEvent::TOUCH_MOVE));
    test.RunTest();
    test.AddExpectation(df::UserEventStream::BEGIN_DRAG);
    test.AddUserEvent(MakeTouchEvent(m2::PointD(threshold + 2, 0), df::TouchEvent::TOUCH_MOVE));
    test.AddExpectation(df::UserEventStream::END_DRAG);
    test.AddUserEvent(MakeTouchEvent(m2::PointD(threshold + 2, 0), df::TouchEvent::TOUCH_UP));
    test.RunTest();
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, TwoFingerThresholdFollowsVisualScale)
{
  SCOPE_GUARD(restoreScale, []() { df::VisualParams::Instance().SetVisualScale(1.0); });
  for (auto const & scales : {std::pair{1.0, 2.0}, std::pair{2.0, 1.0}})
  {
    df::VisualParams::Instance().SetVisualScale(scales.first);
    UserEventStreamTest test;
    test.SetRect(m2::RectD(-250.0, -250.0, 250.0, 250.0));
    test.AddExpectation(df::UserEventStream::TWO_FINGERS_TAP);
    m2::PointD const second(100, 0);
    test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), second, df::TouchEvent::TOUCH_DOWN));
    test.RunTest();

    df::VisualParams::Instance().SetVisualScale(scales.second);
    double const threshold = 10.0 * scales.second;
    test.AddUserEvent(MakeTouchEvent(m2::PointD(threshold - 1, 0), second, df::TouchEvent::TOUCH_MOVE));
    test.AddUserEvent(MakeTouchEvent(m2::PointD(threshold, 0), second, df::TouchEvent::TOUCH_MOVE));
    test.RunTest();
    test.AddExpectation(df::UserEventStream::BEGIN_SCALE);
    test.AddUserEvent(MakeTouchEvent(m2::PointD(threshold + 1, 0), second, df::TouchEvent::TOUCH_MOVE));
    test.AddExpectation(df::UserEventStream::END_SCALE);
    test.AddUserEvent(MakeTouchEvent(m2::PointD(threshold + 1, 0), second, df::TouchEvent::TOUCH_UP));
    test.RunTest();
  }
}
#endif

UNIT_CLASS_TEST(VisualParamsFixture, SetCenter_AlignsToVisibleViewportCenter)
{
  // With an asymmetric visible viewport (e.g. host UI panel covering bottom 30%),
  // SetCenter must place the geographic target at the visible-viewport center
  // regardless of trackVisibleViewport. The flag only controls whether the engine
  // continues to follow the viewport when it changes later.
  uint32_t const kW = 1000, kH = 1000;
  m2::RectD const kVisibleViewport(0.0, 0.0, static_cast<double>(kW), 700.0);
  m2::PointD const kTarget(0.0, 0.0);
  double const kEps = 1.0;
  m2::PointD const viewportCenter = kVisibleViewport.Center();

  for (bool trackVisibleViewport : {false, true})
  {
    UserEventStreamTest test;
    test.AddResizeEvent(kW, kH);
    test.AddSetVisibleViewport(kVisibleViewport);
    test.SetRect(m2::RectD(-100.0, -100.0, 100.0, 100.0));
    test.RunTest();

    test.AddSetCenter(kTarget, df::kDoNotChangeZoom, trackVisibleViewport);
    test.RunTest();

    m2::PointD const pixelPos = test.GetScreen().GtoP(kTarget);
    TEST(std::fabs(pixelPos.x - viewportCenter.x) < kEps,
         (pixelPos.x, "expected", viewportCenter.x, "trackVisibleViewport", trackVisibleViewport));
    TEST(std::fabs(pixelPos.y - viewportCenter.y) < kEps,
         (pixelPos.y, "expected", viewportCenter.y, "trackVisibleViewport", trackVisibleViewport));
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, ShowRect_FarAnimatedMoveEndsNorthUp)
{
  // A move too long for a plain animation zooms out, moves and zooms in. It must also turn the map to the target
  // angle, or the rect ends rotated with its corners outside the visible viewport.
  m2::RectD const kVisibleViewport(0.0, 0.0, 1080.0, 1400.0);
  m2::RectD const italy = RectFromLatLon(36.6, 6.6, 47.1, 18.5);
  m2::PointD const sanFrancisco = mercator::FromLatLon(37.7749, -122.4194);

  UserEventStreamTest test;
  test.AddResizeEvent(1080, 2160);
  test.AddSetVisibleViewport(kVisibleViewport);
  test.SetAnyRect(m2::AnyRectD(sanFrancisco, ang::AngleD(math::pi / 4), m2::RectD(-0.05, -0.1, 0.05, 0.1)));
  test.RunTest();
  TEST(AlmostEqualAbs(test.GetScreen().GetAngle(), math::pi / 4, 1e-6), (test.GetScreen().GetAngle()));

  test.ShowRect(italy, true /* isAnim */);
  test.RunTest();
  test.FinishAnimations();

  TEST(AlmostEqualAbs(test.GetScreen().GetAngle(), 0.0, 1e-6), (test.GetScreen().GetAngle()));
  TestRectIsVisible(test.GetScreen(), italy, kVisibleViewport);
  TestRectFills(test.GetScreen(), italy, kVisibleViewport);
  TestPointIsAt(test.GetScreen(), italy.Center(), kVisibleViewport.Center());
}

UNIT_CLASS_TEST(VisualParamsFixture, ShowRect_PointIsCenteredAtMaxZoom)
{
  // A point gets the closest zoom allowed, centered in the visible viewport. The zoom must be limited before the point
  // is centered, or limiting it later moves the point away from the center, and it must come from the point itself, not
  // from a rect widened to that zoom, which a narrow visible viewport can't fit.
  struct Case
  {
    uint32_t m_tileSize;
    m2::RectD m_view;
    m2::RectD m_visibleViewport;
  };
  Case const cases[] = {
      {512, m2::RectD(0.0, 0.0, 1000.0, 1400.0), m2::RectD(0.0, 0.0, 1000.0, 1000.0)},
      {1024, m2::RectD(0.0, 0.0, 1080.0, 2160.0), m2::RectD(810.0, 0.0, 1080.0, 2160.0)},
  };
  SCOPE_GUARD(restoreParams, []() { df::VisualParams::Init(1.0 /* visualScale */, 1024 /* tileSize */); });
  m2::PointD const colosseum = mercator::FromLatLon(41.8902, 12.4922);
  m2::RectD const rect(colosseum, colosseum);

  for (auto const & c : cases)
  {
    df::VisualParams::Init(1.0 /* visualScale */, c.m_tileSize);
    for (bool isAnim : {false, true})
    {
      UserEventStreamTest test;
      test.AddResizeEvent(static_cast<uint32_t>(c.m_view.SizeX()), static_cast<uint32_t>(c.m_view.SizeY()));
      test.AddSetVisibleViewport(c.m_visibleViewport);
      test.SetRect(RectFromLatLon(41.0, 12.0, 42.0, 13.0));
      test.RunTest();

      test.ShowRect(rect, isAnim);
      test.RunTest();
      test.FinishAnimations();

      TEST_EQUAL(df::GetDrawTileScale(test.GetScreen()), scales::GetUpperStyleScale(), ());
      TestPointIsAt(test.GetScreen(), colosseum, c.m_visibleViewport.Center());
    }
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, ShowRect_FitsInPerspective)
{
  // In perspective the near part of the map is drawn larger and the far part smaller than in the map plane, so the
  // rect fitted in the map plane can be drawn outside the visible viewport, or much smaller than it.
  struct Case
  {
    m2::RectD m_visibleViewport;
    m2::RectD m_rect;
    // A scale at which the rect is known to fit, 0 if none.
    double m_fitScale = 0.0;
  };
  Case const cases[] = {
      // Drawn below the visible viewport.
      {m2::RectD(0.0, 1360.0, 1080.0, 2160.0), RectFromLatLon(41.8864777627, 12.4872, 41.8939220204, 12.4972)},
      // A narrow visible viewport.
      {m2::RectD(0.0, 0.0, 270.0, 2160.0), RectFromLatLon(41.8827553086, 12.4912, 41.8976438239, 12.4932)},
      // Drawn smaller than the whole view.
      {m2::RectD(0.0, 0.0, 1080.0, 2160.0), RectFromLatLon(41.8890833516, 12.4907, 41.8913166289, 12.4937)},
      // A shallow visible viewport, where the scales that fit form two intervals: the closest one must be found.
      {m2::RectD(0.0, 0.0, 1080.0, 560.0), RectFromLatLon(41.88674, 12.4897, 41.89366, 12.4947)},
      // The same, where the closest interval is narrower than 1/70 of a zoom level, and the plane fit fills the height.
      {m2::RectD(0.0, 0.0, 1080.0, 560.0), RectFromLatLon(41.8863075, 12.4897, 41.8940925, 12.4947), 7.33893e-6},
  };

  for (auto const & c : cases)
  {
    for (bool isAnim : {false, true})
    {
      UserEventStreamTest test;
      test.AddResizeEvent(1080, 2160);
      test.AddSetVisibleViewport(c.m_visibleViewport);
      test.SetRect(RectFromLatLon(41.885, 12.488, 41.892, 12.495));
      test.SetAutoPerspective(true);
      test.RunTest();
      TEST(test.GetScreen().isPerspective(), ());

      test.ShowRect(c.m_rect, isAnim);
      test.RunTest();
      test.FinishAnimations();

      TEST(test.GetScreen().isPerspective(), ("The test needs a target in perspective"));
      TestRectIsVisible(test.GetScreen(), c.m_rect, c.m_visibleViewport);
      TestRectFills(test.GetScreen(), c.m_rect, c.m_visibleViewport);
      if (c.m_fitScale > 0.0)
        TEST_LESS_OR_EQUAL(test.GetScreen().GetScale(), c.m_fitScale, ());
    }
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, ShowRect_FitsFlatBeyondTheZoomsInPerspective)
{
  // A large rect fits where the automatic perspective is off. Its plane fit can put a corner a rounding error outside
  // the viewport, which must not make the search fail.
  m2::RectD const kVisibleViewport(0.0, 0.0, 1080.0, 1400.0);
  m2::RectD const italy = RectFromLatLon(36.6, 6.6, 47.1, 18.5);

  for (bool isAnim : {false, true})
  {
    UserEventStreamTest test;
    test.AddResizeEvent(1080, 2160);
    test.AddSetVisibleViewport(kVisibleViewport);
    test.SetRect(RectFromLatLon(41.885, 12.488, 41.892, 12.495));
    test.SetAutoPerspective(true);
    test.RunTest();

    test.ShowRect(italy, isAnim);
    test.RunTest();
    test.FinishAnimations();

    TEST(!test.GetScreen().isPerspective(), ());
    TestRectIsVisible(test.GetScreen(), italy, kVisibleViewport);
    TestRectFills(test.GetScreen(), italy, kVisibleViewport);
    TestPointIsAt(test.GetScreen(), italy.Center(), kVisibleViewport.Center());
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, ShowRect_FitsInTheDrawnPartOfTheVisibleViewport)
{
  // Only the top 60 px of the visible viewport are inside the view.
  m2::RectD const kVisibleViewport(0.0, 2100.0, 1080.0, 3100.0);
  m2::RectD const kDrawnViewport(0.0, 2100.0, 1080.0, 2160.0);
  m2::RectD const rect = RectFromLatLon(41.889, 12.490, 41.892, 12.494);

  for (bool isPerspective : {false, true})
  {
    for (bool isAnim : {false, true})
    {
      UserEventStreamTest test;
      test.AddResizeEvent(1080, 2160);
      test.AddSetVisibleViewport(kVisibleViewport);
      test.SetRect(RectFromLatLon(41.885, 12.488, 41.892, 12.495));
      test.SetAutoPerspective(isPerspective);
      test.RunTest();

      test.ShowRect(rect, isAnim);
      test.RunTest();
      test.FinishAnimations();

      TestRectIsVisible(test.GetScreen(), rect, kDrawnViewport);
      TestRectFills(test.GetScreen(), rect, kDrawnViewport);
    }
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, ShowRect_StaysInsideTheWorldNearThePole)
{
  // Centering a rect near the pole in a low visible viewport would move the view beyond the pole, and stopping it there
  // would leave the rect under what covers the rest of the view: the map zooms in until the rect's center can stay at
  // the center of the visible viewport.
  m2::RectD const kVisibleViewport(0.0, 1728.0, 1080.0, 1944.0);
  m2::RectD const rect = RectFromLatLon(84.99, 12.49, 85.01, 12.51);

  for (bool isAnim : {false, true})
  {
    UserEventStreamTest test;
    test.AddResizeEvent(1080, 2160);
    test.AddSetVisibleViewport(kVisibleViewport);
    test.SetRect(RectFromLatLon(-41.0, 100.0, -40.0, 101.0));
    test.RunTest();

    test.ShowRect(rect, isAnim);
    test.RunTest();
    test.FinishAnimations();

    ScreenBase const & screen = test.GetScreen();
    TEST(df::CheckBorders(screen), (screen.ClipRect()));
    m2::PointD const center(mercator::NearestWrapX(rect.Center().x, screen.GetOrg().x), rect.Center().y);
    TestPointIsAt(screen, center, kVisibleViewport.Center());
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, ShowRect_RotationKeepsTheScale)
{
  // Turning a map that fills the world to north up hits the world's borders in the frames in between. The animation
  // must still end at the requested scale, or the rect is cut off.
  m2::RectD const kViewport(0.0, 0.0, 1080.0, 2160.0);
  m2::RectD const rect = RectFromLatLon(-40.0, -86.4, 40.0, 86.4);

  UserEventStreamTest test;
  test.AddResizeEvent(1080, 2160);
  test.SetAnyRect(m2::AnyRectD(m2::PointD::Zero(), ang::AngleD(math::pi / 2), m2::RectD(-86.4, -172.8, 86.4, 172.8)));
  test.RunTest();

  test.ShowRect(rect, true /* isAnim */);
  test.RunTest();
  test.FinishAnimations();

  TEST(AlmostEqualAbs(test.GetScreen().GetAngle(), 0.0, 1e-6), (test.GetScreen().GetAngle()));
  TestRectIsVisible(test.GetScreen(), rect, kViewport);
  TestRectFills(test.GetScreen(), rect, kViewport);
}
}  // namespace user_event_stream_test
