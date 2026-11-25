#include "testing/testing.hpp"

#include "drape_frontend/animation/arrow_animation.hpp"
#include "drape_frontend/animation/interpolation_holder.hpp"
#include "drape_frontend/animation/show_hide_animation.hpp"
#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "geometry/angles.hpp"

#include "base/scope_guard.hpp"
#include "base/thread.hpp"

#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <list>
#include <utility>

namespace user_event_stream_test
{
using df::test_support::VisualParamsFixture;
using namespace std::placeholders;
using namespace std::chrono_literals;

class CameraAnimationFixture : public VisualParamsFixture
{
public:
  CameraAnimationFixture()
  {
    AddEvent<df::ResizeEvent>(800, 600);
    auto const time =
        AddEvent<df::SetRectEvent>(m2::RectD(-2.0, -2.0, 2.0, 2.0), false, df::kDoNotChangeZoom, false, false);
    Process(time);
  }

  template <typename Event, typename... Args>
  df::AnimationClock::time_point AddEvent(Args &&... args)
  {
    auto event = make_unique_dp<Event>(std::forward<Args>(args)...);
    auto const time = event->GetRequestTime();
    m_stream.AddEvent(std::move(event));
    return time;
  }

  void Finger(df::TouchEvent::ETouchType type, float x = 300, float y = 300)
  {
    auto touch = make_unique_dp<df::TouchEvent>();
    touch->SetTouchType(type);
    touch->SetFirstTouch({m2::PointF(x, y), 1});
    touch->SetFirstMaskedPointer(0);
    m_stream.AddEvent(std::move(touch));
  }

  ScreenBase const & Process(df::AnimationClock::time_point time)
  {
    bool changed, resized, active;
    return m_stream.ProcessEvents(changed, resized, active, time);
  }

  static df::AnimationClock::time_point At(df::AnimationClock::time_point time, double seconds)
  {
    return time + std::chrono::duration_cast<df::AnimationClock::duration>(std::chrono::duration<double>(seconds));
  }

protected:
  df::AnimationSystem m_animations;
  df::UserEventStream m_stream{m_animations};
};

UNIT_CLASS_TEST(CameraAnimationFixture, QueuedZoomReachesTargetInFirstLateFrame)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  m2::PointD const pixel(205, 227);
  auto const point = start.PtoG(pixel);
  auto const requestedAt = AddEvent<df::ScaleEvent>(2.0, pixel, true);

  auto const & screen = Process(requestedAt + 500ms);
  TEST_ALMOST_EQUAL_ABS(screen.GetScale(), start.GetScale() / 2.0, 1e-10, ());
  TEST(screen.GtoP(point).EqualDxDy(pixel, 1e-6), (screen.GtoP(point), pixel));
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, RepeatedZoomAccumulatesBeforeFirstFrame)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  m2::PointD const pixel(290, 335);
  auto const point = start.PtoG(pixel);
  AddEvent<df::ScaleEvent>(2.0, pixel, true);
  auto const requestedAt = AddEvent<df::ScaleEvent>(2.0, pixel, true);

  auto const & intermediate = Process(requestedAt + 70ms);
  TEST_LESS(intermediate.GetScale(), start.GetScale(), ());
  TEST_GREATER(intermediate.GetScale(), start.GetScale() / 4.0, ());
  TEST(intermediate.GtoP(point).EqualDxDy(pixel, 1e-6), (intermediate.GtoP(point), pixel));

  auto const & settled = Process(requestedAt + 400ms);
  TEST_ALMOST_EQUAL_ABS(settled.GetScale(), start.GetScale() / 4.0, 1e-10, ());
  TEST(settled.GtoP(point).EqualDxDy(pixel, 1e-6), (settled.GtoP(point), pixel));
}

UNIT_CLASS_TEST(CameraAnimationFixture, ZoomStepScalesLinearlyForTwoTenthsOfASecond)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  m2::PointD const pixel(205, 227);
  auto const point = start.PtoG(pixel);
  auto const requestedAt = AddEvent<df::ScaleEvent>(2.0, pixel, true);

  auto const & moving = Process(requestedAt + 100ms);
  TEST_ALMOST_EQUAL_ABS(moving.GetScale(), start.GetScale() * 0.75, 1e-10, ());
  TEST(moving.GtoP(point).EqualDxDy(pixel, 1e-6), (moving.GtoP(point), pixel));
  TEST(m_animations.IsCameraMoving(), ());

  auto const & settled = Process(requestedAt + 200ms);
  TEST_ALMOST_EQUAL_ABS(settled.GetScale(), start.GetScale() / 2.0, 1e-10, ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, ZoomOutAtThePoleZoomsOut)
{
  // The top of the screen is 10 px from the north pole, so the zoom-out also moves the map to keep it in the world.
  auto const & world = df::GetWorldRect();
  ScreenBase const start = m_stream.GetCurrentScreen();
  double const top = (start.PixelRect().SizeY() / 2.0 + 10.0) * start.GetScale();
  Process(AddEvent<df::SetCenterEvent>(m2::PointD(0.0, world.maxY() - top), df::kDoNotChangeZoom, false, false));
  auto const center = start.PixelRect().Center();
  auto const time = AddEvent<df::ScaleEvent>(0.5, center, true);

  // Zooming out twice takes 0.2 seconds.
  for (int ms = 0; ms < 200; ms += 30)
    TEST_LESS_OR_EQUAL(Process(time + std::chrono::milliseconds(ms)).ClipRect().maxY(), world.maxY() + 1e-9, (ms));
  auto const & settled = Process(time + 201ms);
  TEST(!m_animations.IsCameraMoving(), ());
  TEST_ALMOST_EQUAL_ABS(settled.GetScale(), start.GetScale() * 2.0, 1e-12, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, PanStepsReplaceThePreviousDestinationAndAccumulate)
{
  auto const centerTime = AddEvent<df::SetCenterEvent>(m2::PointD(3.0, 0.0), df::kDoNotChangeZoom, true, false);
  ScreenBase expected = Process(centerTime + 60ms);
  TEST(m_animations.IsCameraMoving(), ());

  double constexpr kStep = 0.15;
  AddEvent<df::MoveEvent>(kStep, 0.0, true);
  auto const panTime = AddEvent<df::MoveEvent>(kStep, 0.0, true);
  expected.Move(2.0 * kStep * expected.PixelRectIn3d().SizeX(), 0.0);

  // A move takes at least 0.2 seconds.
  auto const & screen = Process(panTime + 201ms);
  TEST(screen.GetOrg().EqualDxDy(expected.GetOrg(), 1e-9), (screen.GetOrg(), expected.GetOrg()));
}

UNIT_CLASS_TEST(CameraAnimationFixture, DragStopsFollowAtDisplayedPosition)
{
  auto const & start = m_stream.GetCurrentScreen();
  auto const anchor = start.PixelRect().Center();
  auto const position = start.PtoG(anchor + m2::PointD(100, 0));
  int const zoom = static_cast<int>(df::GetZoomLevel(start.GetScale())) + 1;
  auto const requestedAt = AddEvent<df::FollowAndRotateEvent>(position, anchor, 0.4, zoom);

  ScreenBase const displayed = Process(requestedAt + 50ms);
  TEST(m_animations.IsCameraMoving(), ());
  Finger(df::TouchEvent::TOUCH_DOWN);
  Finger(df::TouchEvent::TOUCH_MOVE, 330, 300);
  Finger(df::TouchEvent::TOUCH_MOVE, 350, 300);
  auto const & screen = Process(requestedAt + 65ms);
  TEST(m_stream.IsInUserAction(), ());
  TEST(screen == displayed, (screen.GetOrg(), displayed.GetOrg()));
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, TapLetsAutoZoomFinish)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  double const target = start.GetScale() * 0.7;
  auto const time = AddEvent<df::FollowAndRotateEvent>(start.GetOrg(), start.PixelRect().Center(), 0.0, target);
  Process(time + 60ms);
  Finger(df::TouchEvent::TOUCH_DOWN);
  Finger(df::TouchEvent::TOUCH_UP);
  Process(time + 85ms);
  TEST(m_animations.IsCameraMoving(), ());
  // An auto-zoom by 1/0.7 takes 0.6 / 0.7 seconds.
  TEST_ALMOST_EQUAL_ABS(Process(time + 860ms).GetScale(), target, 1e-10, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, DoubleTapAndHoldTakesOverFromFollow)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  Finger(df::TouchEvent::TOUCH_DOWN);
  Finger(df::TouchEvent::TOUCH_UP);
  Finger(df::TouchEvent::TOUCH_DOWN);
  auto const time =
      AddEvent<df::FollowAndRotateEvent>(start.GetOrg(), start.PixelRect().Center(), 0.0, start.GetScale() * 0.8);
  Finger(df::TouchEvent::TOUCH_MOVE, 300, 335);
  Finger(df::TouchEvent::TOUCH_MOVE, 300, 360);
  auto const & screen = Process(time + 70ms);
  TEST(m_stream.IsInUserAction(), ());
  TEST(!m_animations.IsCameraMoving(), ());
  TEST_LESS(screen.GetScale(), start.GetScale() * 0.5, ());
}

namespace
{
struct SelectionOffsetListener : df::UserEventStream::Listener
{
  void OnTap(m2::PointD const &, bool) override {}
  void OnForceTap(m2::PointD const &) override {}
  void OnDoubleTap(m2::PointD const &) override {}
  void OnTwoFingersTap() override {}
  bool OnSingleTouchFiltrate(m2::PointD const &, df::TouchEvent::ETouchType) override { return false; }
  void OnDragStarted() override {}
  void OnDragEnded(m2::PointD const &) override {}
  void OnScaleStarted() override {}
  void OnRotated() override {}
  void OnScrolled(m2::PointD const &) override {}
  void CorrectScalePoint(m2::PointD &) const override {}
  void CorrectGlobalScalePoint(m2::PointD &) const override {}
  void CorrectScalePoint(m2::PointD &, m2::PointD &) const override {}
  void OnScaleEnded() override {}
  void OnScaleRequested() override {}
  void OnTouchMapAction(df::TouchEvent::ETouchType, bool) override {}
  bool OnNewVisibleViewport(m2::RectD const &, m2::RectD const &, bool needOffset, m2::PointD & offset) override
  {
    offset = m_offset;
    return needOffset;
  }

  m2::PointD const m_offset{0.1, 0.05};
};
}  // namespace

UNIT_CLASS_TEST(CameraAnimationFixture, UnchangedVisibleViewportPreservesCameraAnimation)
{
  auto const viewport = m_stream.GetVisibleViewport();
  m2::PointD const destination(0.5, 0.0);
  auto const time = AddEvent<df::SetCenterEvent>(destination, df::kDoNotChangeZoom, true, true);
  AddEvent<df::SetVisibleViewportEvent>(viewport);

  auto const & moving = Process(time + 50ms);
  TEST(m_animations.IsCameraMoving(), ());
  TEST_LESS(moving.GetOrg().x, destination.x, ());
  TEST(Process(time + 220ms).GetOrg().EqualDxDy(destination, 1e-6), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, ChangedVisibleViewportAnimatesToTheTrackedCenter)
{
  m2::PointD const destination(0.5, 0.0);
  auto const time = AddEvent<df::SetCenterEvent>(destination, df::kDoNotChangeZoom, true, true);
  Process(time + 50ms);

  m2::RectD const viewport(0.0, 0.0, 800.0, 420.0);
  auto const layoutTime = AddEvent<df::SetVisibleViewportEvent>(viewport);
  auto const & moving = Process(layoutTime + 100ms);
  TEST(m_animations.IsCameraMoving(), ());
  TEST_LESS(moving.GetOrg().x, destination.x, ());

  auto const & settled = Process(layoutTime + 200ms);
  TEST(!m_animations.IsCameraMoving(), ());
  TEST(settled.GtoP(destination).EqualDxDy(viewport.Center(), 1e-6), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, VisibleViewportOffsetAnimatesWhileIdle)
{
  SelectionOffsetListener listener;
  m_stream.SetListener(make_ref(&listener));
  auto const start = m_stream.GetCurrentScreen().GetOrg();
  auto const layoutTime = AddEvent<df::SetVisibleViewportEvent>(m2::RectD(0.0, 0.0, 800.0, 420.0));
  auto const & moving = Process(layoutTime + 100ms);
  TEST(m_animations.IsCameraMoving(), ());
  TEST(moving.GetOrg().EqualDxDy(start - listener.m_offset * 0.5, 1e-9), (moving.GetOrg()));
  TEST(Process(layoutTime + 200ms).GetOrg().EqualDxDy(start - listener.m_offset, 1e-9), ());
  m_stream.SetListener(nullptr);
}

UNIT_CLASS_TEST(CameraAnimationFixture, SelectionOffsetMovesTheCameraDestination)
{
  SelectionOffsetListener listener;
  m_stream.SetListener(make_ref(&listener));
  m2::PointD const destination(0.5, 0.0);
  auto const time = AddEvent<df::SetCenterEvent>(destination, df::kDoNotChangeZoom, true, false);
  Process(time + 50ms);
  auto const layoutTime = AddEvent<df::SetVisibleViewportEvent>(m2::RectD(0.0, 0.0, 800.0, 420.0));
  Process(layoutTime + 10ms);
  TEST(m_animations.IsCameraMoving(), ());
  auto const & settled = Process(layoutTime + 200ms);
  TEST(settled.GetOrg().EqualDxDy(destination - listener.m_offset, 1e-6), ());
  m_stream.SetListener(nullptr);
}

UNIT_CLASS_TEST(CameraAnimationFixture, CenterAnimatesPanAndZoomTogether)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const & viewport = start.PixelRect();
  auto const destination = start.PtoG(viewport.Center() + m2::PointD(2.0 * viewport.SizeX(), 0.0));
  double const targetScale = start.GetScale() / 4.0;
  auto const time = AddEvent<df::SetCenterEvent>(4.0, destination, true, false);

  auto const & moving = Process(time + 70ms);
  TEST_GREATER(moving.GetOrg().x, start.GetOrg().x, ());
  TEST_LESS(moving.GetOrg().x, destination.x, ());
  TEST_GREATER(moving.GetScale(), targetScale, ());
  TEST_LESS(moving.GetScale(), start.GetScale(), ());

  // The move at the average scale takes longer than the zoom.
  auto const & settled = Process(time + 610ms);
  TEST(settled.GetOrg().EqualDxDy(destination, 1e-9), ());
  TEST_ALMOST_EQUAL_ABS(settled.GetScale(), targetScale, 1e-10, ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, CenterMovesLinearlyWithinTheTimeLimit)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  // The map moves seven lengths of the shorter viewport side per second.
  double constexpr kDuration = 5600.0 / 4200.0;
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(5600.0, 0.0));
  auto const time = AddEvent<df::SetCenterEvent>(destination, df::kDoNotChangeZoom, true, false);

  auto const & moving = Process(At(time, kDuration * 0.5));
  TEST(moving.GetOrg().EqualDxDy((start.GetOrg() + destination) * 0.5, 1e-6), (moving.GetOrg()));
  TEST_EQUAL(moving.GetScale(), start.GetScale(), ());
  TEST(Process(At(time, kDuration + 0.001)).GetOrg().EqualDxDy(destination, 1e-9), ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, DistantCenterFliesAtTheFlightZoom)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(8000.0, 0.0));
  // The map zooms out in 0.1 s to show the distance as 5 lengths of the shorter viewport side.
  double const flightScale = start.GetScale() * 8000.0 / (5.0 * 600.0);
  // The flight takes 1.1 s and 0.15 s more per zoom level back from the flight scale.
  double const duration = 1.1 + 0.15 * std::log2(flightScale / start.GetScale());
  auto const time = AddEvent<df::SetCenterEvent>(destination, df::kDoNotChangeZoom, true, false);

  double previousOffset = 8000.0;
  for (double t = 0.001; t < duration; t += 0.05)
  {
    auto const & screen = Process(At(time, t));
    if (t >= 0.1 && t < duration * 0.3)
      TEST_ALMOST_EQUAL_ABS(screen.GetScale(), flightScale, 1e-12, (t));
    TEST_LESS_OR_EQUAL(screen.GetScale(), flightScale * (1.0 + 1e-12), (t));
    TEST_GREATER_OR_EQUAL(screen.GetScale(), start.GetScale(), (t));
    double const offset = (destination.x - screen.GetOrg().x) / screen.GetScale();
    TEST_LESS(offset, previousOffset, (t));
    previousOffset = offset;
  }

  auto const & settled = Process(At(time, duration + 0.001));
  TEST(settled.GetOrg().EqualDxDy(destination, 1e-9), ());
  TEST_EQUAL(settled.GetScale(), start.GetScale(), ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, DistantCenterFliesAndZoomsInTogether)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(8000.0, 0.0));
  int const zoom = static_cast<int>(df::GetZoomLevel(start.GetScale())) + 6;
  double const flightScale = start.GetScale() * 8000.0 / (5.0 * 600.0);
  double const endScale = df::GetScreenScale(zoom);
  double const duration = std::min(1.1 + 0.15 * std::log2(flightScale / endScale), 2.0);
  auto const time = AddEvent<df::SetCenterEvent>(destination, zoom, true, false);

  // On arrival, the map zooms in while the destination keeps coming closer.
  double previousScale = flightScale;
  double previousOffset = 5.0 * 600.0;
  for (double t = duration * 0.3 + 0.05; t < duration; t += 0.05)
  {
    auto const & screen = Process(At(time, t));
    TEST(m_animations.IsCameraMoving(), (t));
    TEST_LESS(screen.GetScale(), previousScale, (t));
    TEST_GREATER(screen.GetScale(), endScale, (t));
    double const offset = (destination.x - screen.GetOrg().x) / screen.GetScale();
    TEST_LESS(offset, previousOffset, (t));
    previousScale = screen.GetScale();
    previousOffset = offset;
  }

  auto const & settled = Process(At(time, duration + 0.001));
  TEST(settled.GetOrg().EqualDxDy(destination, 1e-9), ());
  TEST_EQUAL(settled.GetScale(), endScale, ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, DeepZoomWithAShortMoveFlies)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  // The move is short at the start scale, but too long at the average scale of a linear animation.
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(4000.0, 0.0));
  int const zoom = static_cast<int>(df::GetZoomLevel(start.GetScale())) + 9;
  auto const time = AddEvent<df::SetCenterEvent>(destination, zoom, true, false);

  // After the zoom-out, the map shows the distance as 5 lengths of the shorter viewport side.
  double const flightScale = start.GetScale() * 4000.0 / (5.0 * 600.0);
  double const duration = std::min(1.1 + 0.15 * std::log2(flightScale / df::GetScreenScale(zoom)), 2.0);
  auto const & flying = Process(At(time, (0.1 + duration * 0.3) / 2.0));
  TEST_ALMOST_EQUAL_ABS(flying.GetScale(), flightScale, 1e-12, ());
  TEST_GREATER(flying.GetScale(), start.GetScale(), ());
  auto const & settled = Process(At(time, 2.001));
  TEST(settled.GetOrg().EqualDxDy(destination, 1e-9), ());
  TEST_EQUAL(settled.GetScale(), df::GetScreenScale(zoom), ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FlightStartsFromTheDisplayedMap)
{
  // A quarter of the world away at a street zoom.
  Process(AddEvent<df::SetCenterEvent>(m2::PointD::Zero(), 17, false, false));
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const time = AddEvent<df::SetCenterEvent>(m2::PointD(90.0, 0.0), 17, true, false);

  // The first frame still shows the start of the flight.
  auto const & first = Process(At(time, 0.016));
  TEST(m_animations.IsCameraMoving(), ());
  TEST_LESS(first.GtoP(start.GetOrg()).Length(first.PixelRect().Center()), 300.0, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FollowFliesToADistantPosition)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const anchor = start.PixelRect().Center();
  auto const position = start.PtoG(anchor + m2::PointD(8000.0, 0.0));
  int const zoom = static_cast<int>(df::GetZoomLevel(start.GetScale())) + 1;
  auto const time = AddEvent<df::FollowAndRotateEvent>(position, anchor, 0.3, zoom);
  // After the zoom-out, the map shows the distance as 5 lengths of the shorter viewport side.
  double const flightScale = start.GetScale() * 8000.0 / (5.0 * 600.0);
  auto const & flying = Process(At(time, 0.2));
  TEST_ALMOST_EQUAL_ABS(flying.GetScale(), flightScale, 1e-12, ());
  TEST_GREATER(flying.GetScale(), start.GetScale(), ());

  auto const & settled = Process(At(time, 2.001));
  TEST(!m_animations.IsCameraMoving(), ());
  TEST(settled.GtoP(position).EqualDxDy(anchor, 1e-6), (settled.GtoP(position), anchor));
  TEST_EQUAL(settled.GetScale(), df::GetScreenScale(zoom), ());
  TEST_ALMOST_EQUAL_ABS(settled.GetAngle(), -0.3, 1e-10, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, DeepZoomAnimatesWithinTheTimeLimit)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  // Six zoom levels would take 6.4 seconds at the zoom speed.
  auto const time = AddEvent<df::SetCenterEvent>(64.0, start.GetOrg(), true, false);
  auto const & moving = Process(time + 750ms);
  TEST(m_animations.IsCameraMoving(), ());
  TEST_ALMOST_EQUAL_ABS(moving.GetScale(), (start.GetScale() + start.GetScale() / 64.0) * 0.5, 1e-12, ());
  TEST_ALMOST_EQUAL_ABS(Process(time + 1500ms).GetScale(), start.GetScale() / 64.0, 1e-12, ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FollowZoomAnimatesWithinTheTimeLimit)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const anchor = start.PixelRect().Center();
  int const zoom = static_cast<int>(df::GetZoomLevel(start.GetScale())) + 6;
  auto const time = AddEvent<df::FollowAndRotateEvent>(start.GetOrg(), anchor, 0.0, zoom);
  auto const & moving = Process(time + 750ms);
  TEST(m_animations.IsCameraMoving(), ());
  TEST_GREATER(moving.GetScale(), df::GetScreenScale(zoom), ());
  TEST_ALMOST_EQUAL_ABS(Process(time + 1500ms).GetScale(), df::GetScreenScale(zoom), 1e-12, ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, RotationTakesThreeQuartersOfASecondPerHalfTurn)
{
  auto const time = AddEvent<df::RotateEvent>(math::pi2, true);
  TEST_ALMOST_EQUAL_ABS(Process(time + 187500us).GetAngle(), math::pi4, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(Process(time + 375ms).GetAngle(), math::pi2, 1e-9, ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, RotationKeepsTheVisibleViewportCenter)
{
  // A place page on the right.
  m2::RectD const viewport(0.0, 0.0, 500.0, 600.0);
  Process(AddEvent<df::SetVisibleViewportEvent>(viewport));
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const point = start.PtoG(start.P3dtoP(viewport.Center()));

  // A half turn takes 0.75 seconds.
  auto const time = AddEvent<df::RotateEvent>(math::pi, true);
  for (int ms = 50; ms < 750; ms += 50)
  {
    auto const & screen = Process(time + std::chrono::milliseconds(ms));
    TEST(screen.GtoP(point).EqualDxDy(viewport.Center(), 1e-6), (ms, screen.GtoP(point)));
  }
  TEST_ALMOST_EQUAL_ABS(ang::AngleIn2PI(Process(time + 751ms).GetAngle()), math::pi, 1e-9, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, HeadingUpdateEndsTheZoomOnTime)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const center = start.PixelRect().Center();
  // Zooming in twice takes 0.2 seconds, a quarter turn takes 0.375 seconds.
  auto const zoomTime = AddEvent<df::ScaleEvent>(2.0, center, true);
  AddEvent<df::FollowAndRotateEvent>(start.GetOrg(), center, math::pi2, df::kDoNotChangeZoom);
  // One more heading update in the middle of the zoom.
  Process(zoomTime + 100ms);
  AddEvent<df::FollowAndRotateEvent>(start.GetOrg(), center, math::pi2, df::kDoNotChangeZoom);

  auto const & zoomed = Process(zoomTime + 201ms);
  TEST_ALMOST_EQUAL_ABS(zoomed.GetScale(), start.GetScale() / 2.0, 1e-12, ());
  TEST(m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, RotateEventUsesTheSameDirectionInBothProjections)
{
  for (bool perspective : {false, true})
  {
    AddEvent<df::SetCenterEvent>(m2::PointD::Zero(), 17, false, false);
    auto const mode = AddEvent<df::SetAutoPerspectiveEvent>(perspective);
    TEST_EQUAL(Process(mode).isPerspective(), perspective, ());

    for (double angle : {0.3, -0.2})
    {
      auto const time = AddEvent<df::RotateEvent>(angle, false);
      TEST_ALMOST_EQUAL_ABS(Process(time).GetAngle(), angle, 1e-9, (perspective));
    }
  }
}

UNIT_CLASS_TEST(CameraAnimationFixture, AutoZoomContinuesAfterTheArrowArrives)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const anchor = start.PixelRect().Center();
  auto const position = start.PtoG(anchor + m2::PointD(30, 35));
  double const endScale = start.GetScale() * 0.8;
  m_animations.HoldArrowPose({start.GetOrg(), 0.0});
  auto const requestedAt =
      AddEvent<df::FollowAndRotateEvent>(position, anchor, 0.4, endScale, df::ArrowPose{position, 0.4});

  // The camera keeps the arrow at the anchor while the position comes to it.
  auto const & moving = Process(requestedAt + 100ms);
  auto const movingArrow = m_animations.GetArrowPose();
  TEST(movingArrow.has_value(), ());
  TEST(moving.GtoP(movingArrow->m_position).EqualDxDy(anchor, 1e-6), (moving.GtoP(movingArrow->m_position), anchor));
  TEST(!movingArrow->m_position.EqualDxDy(position, 1e-9), ());

  // The position reaches the anchor with the arrow in 0.2 seconds.
  auto const & arrived = Process(requestedAt + 200ms);
  TEST(arrived.GtoP(position).EqualDxDy(anchor, 1e-6), (arrived.GtoP(position), anchor));
  TEST_ALMOST_EQUAL_ABS(arrived.GetAngle(), -0.4, 1e-10, ());
  auto const arrowPose = m_animations.GetArrowPose();
  TEST(arrowPose.has_value(), ());
  TEST(arrowPose->m_position.EqualDxDy(position, 1e-9), (arrowPose->m_position, position));
  TEST(m_animations.IsCameraMoving(), ());
  TEST_GREATER(arrived.GetScale(), endScale, ());

  // An auto-zoom by 1.25 takes 0.6 * 1.25 seconds.
  auto const & settled = Process(requestedAt + 751ms);
  TEST(!m_animations.IsCameraMoving(), ());
  TEST_ALMOST_EQUAL_ABS(settled.GetScale(), endScale, 1e-10, ());
  TEST(settled.GtoP(position).EqualDxDy(anchor, 1e-6), (settled.GtoP(position), anchor));
}

UNIT_CLASS_TEST(CameraAnimationFixture, ArrowStaysAtTheAnchorThroughATurn)
{
  // A position 100 px ahead after a right turn.
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const anchor = start.PixelRect().Center();
  auto const position = start.PtoG(anchor + m2::PointD(0.0, -100.0));
  m_animations.HoldArrowPose({start.GetOrg(), 0.0});
  auto const time = AddEvent<df::FollowAndRotateEvent>(position, anchor, math::pi2, df::kDoNotChangeZoom,
                                                       df::ArrowPose{position, math::pi2});
  // A quarter turn takes 0.375 seconds.
  for (int ms = 20; ms < 375; ms += 40)
  {
    auto const & screen = Process(time + std::chrono::milliseconds(ms));
    auto const arrow = m_animations.GetArrowPose();
    TEST(arrow.has_value(), (ms));
    TEST(screen.GtoP(arrow->m_position).EqualDxDy(anchor, 1e-6), (ms, screen.GtoP(arrow->m_position)));
    // The arrow turns with the map.
    TEST_ALMOST_EQUAL_ABS(ang::AngleIn2PI(arrow->m_azimuth + screen.GetAngle()), 0.0, 1e-9, (ms));
  }
}

UNIT_CLASS_TEST(CameraAnimationFixture, QueuedArrowTargetStartsAtThePreviousEndpoint)
{
  auto const origin = m_stream.GetCurrentScreen().GetOrg();
  df::ArrowPose const previousTarget{origin + m2::PointD(0.2, 0.0), 0.3};
  auto const previousTime = df::AnimationClock::now() - 300ms;
  m_animations.HoldArrowPose({origin, 0.0});
  m_animations.AnimateArrowTo(previousTarget, previousTime, 0.2, 0.2);
  Process(previousTime + 50ms);
  auto const moving = m_animations.GetArrowPose();
  TEST(moving.has_value(), ());
  m_animations.HoldArrowPose(*moving);
  Process(previousTime + 200ms);
  Process(previousTime + 205ms);

  df::ArrowPose const nextTarget{origin + m2::PointD(0.4, 0.0), 0.6};
  auto const time = AddEvent<df::SetCenterEvent>(nextTarget.m_position, df::kDoNotChangeZoom, true, false, nextTarget);
  Process(time);
  auto const start = m_animations.GetArrowPose();
  TEST(start.has_value(), ());
  TEST_EQUAL(start->m_position, previousTarget.m_position, ());
  TEST_EQUAL(start->m_azimuth, previousTarget.m_azimuth, ());
  Process(time + 200ms);
  auto const settled = m_animations.GetArrowPose();
  TEST(settled.has_value(), ());
  TEST_EQUAL(settled->m_position, nextTarget.m_position, ());
  TEST_EQUAL(settled->m_azimuth, nextTarget.m_azimuth, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FlingNeedsAMovingFinger)
{
  df::KineticScroller scroller;
  ScreenBase screen = m_stream.GetCurrentScreen();
  auto const start = df::AnimationClock::now();
  scroller.Init(screen, start);
  // A fast drag, then the finger stays still before the release. The average speed of 940 px/s is above 200 px/s.
  screen.SetOrg(screen.GetOrg() + m2::PointD(300.0 * screen.GetScale(), 0));
  scroller.Update(screen, start + 20ms);
  TEST(!scroller.CreateKineticAnimation(screen, start + 320ms).has_value(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FlingGlidesForOneAndAHalfSeconds)
{
  df::KineticScroller scroller;
  ScreenBase screen = m_stream.GetCurrentScreen();
  auto const start = df::AnimationClock::now();
  scroller.Init(screen, start);
  double const scale = screen.GetScale();
  screen.SetOrg(screen.GetOrg() + m2::PointD(10.0 * scale, 0));
  auto const release = start + 20ms;
  scroller.Update(screen, release);
  auto fling = scroller.CreateKineticAnimation(screen, release);
  TEST(fling.has_value(), ());

  // The release speed of 500 px/s carries the map 200 px, and the speed decays exponentially.
  double constexpr kDistance = 200.0;
  ScreenBase sample = screen;
  fling->Advance(release + 1ms);
  fling->Apply(sample);
  double const initialSpeed = sample.GetOrg().Length(screen.GetOrg()) / scale / 0.001;
  TEST_ALMOST_EQUAL_ABS(initialSpeed, kDistance * 4.0 / 1.5 / (1.0 - std::exp(-4.0)), 1.0, ());

  fling->Advance(release + 1499ms);
  TEST(!fling->IsFinished(), ());
  fling->Advance(release + 1500ms);
  TEST(fling->IsFinished(), ());
  fling->Apply(sample);
  double const distance = sample.GetOrg().Length(screen.GetOrg()) / scale;
  TEST_ALMOST_EQUAL_ABS(distance, kDistance, 1e-6, ());
}

UNIT_TEST(MoveDurationGrowsWithTheDistance)
{
  // A move covers 7 lengths of the shorter viewport side per second and takes at least 0.2 seconds.
  m2::RectD const viewport(0.0, 0.0, 800.0, 600.0);
  TEST_EQUAL(df::GetMoveDuration(0.0, viewport, 1.0 /* scale */), 0.0, ());
  double previous = 0.2;
  for (double pixels = 1.0; pixels < 3000.0; pixels += 10.0)
  {
    double const duration = df::GetMoveDuration(pixels, viewport, 1.0 /* scale */);
    TEST_GREATER_OR_EQUAL(duration, previous, (pixels));
    previous = duration;
  }
  TEST_ALMOST_EQUAL_ABS(df::GetMoveDuration(3000.0, viewport, 1.0 /* scale */), 5.0 / 7.0, 1e-12, ());
}

UNIT_TEST(OverlayFadeReachesEndpointAfterStalledFrame)
{
  df::ShowHideAnimation fade(false, df::kOverlayFadeTimeSec);
  fade.AnimateVisibility(true);
  df::InterpolationHolder::Instance().Advance(df::AnimationClock::now() + 300ms);
  TEST_EQUAL(fade.GetVisibility(), 1.0, ());
  fade.AnimateVisibility(false);
  fade.AnimateVisibility(true);
  TEST(fade.IsFinished(), ());
  TEST_EQUAL(fade.GetVisibility(), 1.0, ());
}

#ifdef DEBUG

namespace
{
class UserEventStreamTest : df::UserEventStream::Listener
{
public:
  explicit UserEventStreamTest(bool filtrateTouches) : m_filtrate(filtrateTouches)
  {
    m_stream.SetTestBridge(std::bind(&UserEventStreamTest::TestBridge, this, _1));
  }

  void OnTap(m2::PointD const & pt, bool isLong) override {}
  void OnForceTap(m2::PointD const & pt) override {}
  void OnDoubleTap(m2::PointD const & pt) override {}
  void OnTwoFingersTap() override {}
  bool OnSingleTouchFiltrate(m2::PointD const & pt, df::TouchEvent::ETouchType type) override { return m_filtrate; }
  void OnDragStarted() override {}
  void OnDragEnded(m2::PointD const & /* distance */) override {}
  void OnRotated() override {}
  void OnScrolled(m2::PointD const & distance) override {}

  void OnScaleStarted() override {}
  void CorrectScalePoint(m2::PointD & pt) const override {}
  void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const override {}
  void CorrectGlobalScalePoint(m2::PointD & pt) const override {}
  void OnScaleEnded() override {}
  void OnTouchMapAction(df::TouchEvent::ETouchType touchType, bool isMapTouch) override {}
  void OnScaleRequested() override {}
  bool OnNewVisibleViewport(m2::RectD const & oldViewport, m2::RectD const & newViewport, bool needOffset,
                            m2::PointD & gOffset) override
  {
    return false;
  }

  void AddUserEvent(df::TouchEvent const & event) { m_stream.AddEvent(make_unique_dp<df::TouchEvent>(event)); }

  void SetRect(m2::RectD const & r)
  {
    m_stream.AddEvent(make_unique_dp<df::SetRectEvent>(r, false /* rotate */, -1, false /* isAnim */,
                                                       false /* useVisibleViewport */));
  }

  void AddResizeEvent(uint32_t w, uint32_t h) { m_stream.AddEvent(make_unique_dp<df::ResizeEvent>(w, h)); }

  void AddSetVisibleViewport(m2::RectD const & rect)
  {
    m_stream.AddEvent(make_unique_dp<df::SetVisibleViewportEvent>(rect));
  }

  void AddSetCenter(m2::PointD const & center, int zoom, bool trackVisibleViewport)
  {
    m_stream.AddEvent(make_unique_dp<df::SetCenterEvent>(center, zoom, false /* isAnim */, trackVisibleViewport));
  }

  ScreenBase const & GetScreen() const { return m_stream.GetCurrentScreen(); }

  void AddExpectation(char const * action) { m_expectation.push_back(action); }

  void RunTest()
  {
    bool dummy1, dummy2, dummy3;
    m_stream.ProcessEvents(dummy1, dummy2, dummy3, df::AnimationClock::now());
    TEST_EQUAL(m_expectation.empty(), true, ());
  }

private:
  void TestBridge(char const * action)
  {
    TEST(!m_expectation.empty(), ());
    char const * a = m_expectation.front();
    TEST_EQUAL(strcmp(action, a), 0, ());
    m_expectation.pop_front();
  }

private:
  df::AnimationSystem m_animations;
  df::UserEventStream m_stream{m_animations};
  std::list<char const *> m_expectation;
  bool m_filtrate;
};

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
}  // namespace

UNIT_CLASS_TEST(VisualParamsFixture, SimpleTap)
{
  UserEventStreamTest test(false);
  test.AddExpectation(df::UserEventStream::TRY_FILTER);
  test.AddExpectation(df::UserEventStream::BEGIN_TAP_DETECTOR);
  test.AddExpectation(df::UserEventStream::SHORT_TAP_DETECTED);

  test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_DOWN));
  test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_UP));

  test.RunTest();
}

UNIT_CLASS_TEST(VisualParamsFixture, SimpleLongTap)
{
  UserEventStreamTest test(false);
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
  UserEventStreamTest test(false);
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
  UserEventStreamTest test(false);
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
    UserEventStreamTest test(false);
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
    UserEventStreamTest test(false);
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
    UserEventStreamTest test(false);
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

#endif
}  // namespace user_event_stream_test
