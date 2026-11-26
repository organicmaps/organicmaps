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

namespace
{
// The shorter side of the CameraAnimationFixture viewport.
double constexpr kViewportSide = 600.0;

// The time of a move over viewportSides lengths of the shorter viewport side that changes the zoom by zoomLevels.
double GetMoveTime(double viewportSides, double zoomLevels)
{
  double const pan =
      viewportSides == 0.0 ? 0.0 : std::max(df::kShortMoveTimeSec, viewportSides / df::kMoveViewportsPerSec);
  return std::min(std::max(pan, zoomLevels * df::kMoveTimeSecPerZoomLevel), df::kMaxMoveTimeSec);
}

// The time of a flight that changes the zoom by zoomLevels on arrival.
double GetFlyTime(double zoomLevels)
{
  return std::min(df::kMinFlyTimeSec + df::kFlyTimeSecPerZoomLevel * zoomLevels, df::kMaxFlyTimeSec);
}

// The scale of a flight over the given pixels of the screen.
double GetFlightScale(ScreenBase const & screen, double pixels)
{
  return screen.GetScale() * pixels / (df::kFlyViewports * kViewportSide);
}

// The time of a zoom step by the scale ratio.
double GetZoomTime(double ratio)
{
  return ratio * df::kZoomTimeSecPerScaleRatio;
}

// The zoom level that is levels away from the one of the screen, as search and bookmarks request.
int GetZoom(ScreenBase const & screen, int levels)
{
  return static_cast<int>(df::GetZoomLevel(screen.GetScale())) + levels;
}

// The zoom levels between the screen and the zoom.
double GetZoomLevels(ScreenBase const & screen, int zoom)
{
  return std::fabs(std::log2(screen.GetScale() / df::GetScreenScale(zoom)));
}
}  // namespace

class CameraAnimationFixture : public VisualParamsFixture
{
public:
  CameraAnimationFixture()
  {
    AddEvent<df::ResizeEvent>(800, static_cast<int>(kViewportSide));
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

  df::AnimationSystem m_animations;
  df::UserEventStream m_stream{m_animations};
};

UNIT_CLASS_TEST(CameraAnimationFixture, QueuedZoomReachesTargetInFirstLateFrame)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  m2::PointD const pixel(205, 227);
  auto const point = start.PtoG(pixel);
  auto const requestedAt = AddEvent<df::ScaleEvent>(2.0, pixel, true);

  auto const & screen = Process(At(requestedAt, GetZoomTime(2.0) + 0.3));
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

  auto const & intermediate = Process(At(requestedAt, GetZoomTime(4.0) * 0.2));
  TEST_LESS(intermediate.GetScale(), start.GetScale(), ());
  TEST_GREATER(intermediate.GetScale(), start.GetScale() / 4.0, ());
  TEST(intermediate.GtoP(point).EqualDxDy(pixel, 1e-6), (intermediate.GtoP(point), pixel));

  auto const & settled = Process(At(requestedAt, GetZoomTime(4.0) + 0.001));
  TEST_ALMOST_EQUAL_ABS(settled.GetScale(), start.GetScale() / 4.0, 1e-10, ());
  TEST(settled.GtoP(point).EqualDxDy(pixel, 1e-6), (settled.GtoP(point), pixel));
}

UNIT_CLASS_TEST(CameraAnimationFixture, ZoomStepScalesLinearly)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  m2::PointD const pixel(205, 227);
  auto const point = start.PtoG(pixel);
  auto const requestedAt = AddEvent<df::ScaleEvent>(2.0, pixel, true);

  auto const & moving = Process(At(requestedAt, GetZoomTime(2.0) * 0.5));
  TEST_ALMOST_EQUAL_ABS(moving.GetScale(), start.GetScale() * 0.75, 1e-10, ());
  TEST(moving.GtoP(point).EqualDxDy(pixel, 1e-6), (moving.GtoP(point), pixel));
  TEST(m_animations.IsCameraMoving(), ());

  auto const & settled = Process(At(requestedAt, GetZoomTime(2.0) + 0.001));
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

  for (double t = 0.0; t < GetZoomTime(2.0); t += 0.03)
    TEST_LESS_OR_EQUAL(Process(At(time, t)).ClipRect().maxY(), world.maxY() + 1e-9, (t));
  auto const & settled = Process(At(time, GetZoomTime(2.0) + 0.001));
  TEST(!m_animations.IsCameraMoving(), ());
  TEST_ALMOST_EQUAL_ABS(settled.GetScale(), start.GetScale() * 2.0, 1e-12, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, PanStepsReplaceThePreviousDestinationAndAccumulate)
{
  // Three quarters of the shorter viewport side.
  auto const centerTime = AddEvent<df::SetCenterEvent>(m2::PointD(3.0, 0.0), df::kDoNotChangeZoom, true, false);
  ScreenBase expected = Process(At(centerTime, GetMoveTime(0.75, 0.0) * 0.5));
  TEST(m_animations.IsCameraMoving(), ());

  // The second step moves the destination of the first one and keeps its time.
  double constexpr kStep = 0.1;
  AddEvent<df::MoveEvent>(kStep, 0.0, true);
  auto const panTime = AddEvent<df::MoveEvent>(kStep, 0.0, true);
  double const stepPixels = kStep * expected.PixelRectIn3d().SizeX();
  expected.Move(2.0 * stepPixels, 0.0);

  auto const & screen = Process(At(panTime, GetMoveTime(stepPixels / kViewportSide, 0.0) + 0.001));
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
  double const duration = df::kAutoZoomTimeSecPerScaleRatio / 0.7;
  TEST_ALMOST_EQUAL_ABS(Process(At(time, duration + 0.001)).GetScale(), target, 1e-10, ());
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

  auto const & moving = Process(At(time, df::kShortMoveTimeSec * 0.25));
  TEST(m_animations.IsCameraMoving(), ());
  TEST_LESS(moving.GetOrg().x, destination.x, ());
  TEST(Process(At(time, df::kShortMoveTimeSec + 0.001)).GetOrg().EqualDxDy(destination, 1e-6), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, ChangedVisibleViewportAnimatesToTheTrackedCenter)
{
  m2::PointD const destination(0.5, 0.0);
  auto const time = AddEvent<df::SetCenterEvent>(destination, df::kDoNotChangeZoom, true, true);
  Process(At(time, df::kShortMoveTimeSec * 0.25));

  m2::RectD const viewport(0.0, 0.0, 800.0, 420.0);
  auto const layoutTime = AddEvent<df::SetVisibleViewportEvent>(viewport);
  auto const & moving = Process(At(layoutTime, df::kShortMoveTimeSec * 0.5));
  TEST(m_animations.IsCameraMoving(), ());
  TEST_LESS(moving.GetOrg().x, destination.x, ());

  auto const & settled = Process(At(layoutTime, df::kShortMoveTimeSec + 0.001));
  TEST(!m_animations.IsCameraMoving(), ());
  TEST(settled.GtoP(destination).EqualDxDy(viewport.Center(), 1e-6), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, VisibleViewportOffsetAnimatesWhileIdle)
{
  SelectionOffsetListener listener;
  m_stream.SetListener(make_ref(&listener));
  auto const start = m_stream.GetCurrentScreen().GetOrg();
  auto const layoutTime = AddEvent<df::SetVisibleViewportEvent>(m2::RectD(0.0, 0.0, 800.0, 420.0));
  auto const & moving = Process(At(layoutTime, df::kShortMoveTimeSec * 0.5));
  TEST(m_animations.IsCameraMoving(), ());
  // The move eases out: more than a half of the offset in a half of the time.
  double const moved = moving.GetOrg().Length(start) / listener.m_offset.Length();
  TEST(moved > 0.5 && moved < 1.0, (moved));
  TEST(moving.GetOrg().EqualDxDy(start - listener.m_offset * moved, 1e-9), (moving.GetOrg()));
  TEST(Process(At(layoutTime, df::kShortMoveTimeSec + 0.001)).GetOrg().EqualDxDy(start - listener.m_offset, 1e-9), ());
  m_stream.SetListener(nullptr);
}

UNIT_CLASS_TEST(CameraAnimationFixture, UnchangedVisibleViewportMovesANewSelectionIntoView)
{
  // A new selection sends the same visible viewport again to move the selection into it.
  SelectionOffsetListener listener;
  m_stream.SetListener(make_ref(&listener));
  auto const start = m_stream.GetCurrentScreen().GetOrg();
  auto const time = AddEvent<df::SetVisibleViewportEvent>(m_stream.GetVisibleViewport());
  TEST(Process(At(time, df::kShortMoveTimeSec + 0.001)).GetOrg().EqualDxDy(start - listener.m_offset, 1e-9), ());
  m_stream.SetListener(nullptr);
}

UNIT_CLASS_TEST(CameraAnimationFixture, SelectionOffsetMovesTheCameraDestination)
{
  SelectionOffsetListener listener;
  m_stream.SetListener(make_ref(&listener));
  m2::PointD const destination(0.5, 0.0);
  auto const time = AddEvent<df::SetCenterEvent>(destination, df::kDoNotChangeZoom, true, false);
  Process(At(time, df::kShortMoveTimeSec * 0.25));
  auto const layoutTime = AddEvent<df::SetVisibleViewportEvent>(m2::RectD(0.0, 0.0, 800.0, 420.0));
  Process(At(layoutTime, df::kShortMoveTimeSec * 0.3));
  TEST(m_animations.IsCameraMoving(), ());
  auto const & settled = Process(At(layoutTime, df::kShortMoveTimeSec + 0.001));
  TEST(settled.GetOrg().EqualDxDy(destination - listener.m_offset, 1e-6), ());
  m_stream.SetListener(nullptr);
}

UNIT_CLASS_TEST(CameraAnimationFixture, CenterAnimatesPanAndZoomTogether)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const & viewport = start.PixelRect();
  auto const destination = start.PtoG(viewport.Center() + m2::PointD(2.0 * viewport.SizeX(), 0.0));
  int const zoom = GetZoom(start, 2);
  double const targetScale = df::GetScreenScale(zoom);
  double const duration = GetMoveTime(2.0 * viewport.SizeX() / kViewportSide, GetZoomLevels(start, zoom));
  auto const time = AddEvent<df::SetCenterEvent>(destination, zoom, true, false);

  auto const & moving = Process(At(time, duration * 0.1));
  TEST_GREATER(moving.GetOrg().x, start.GetOrg().x, ());
  TEST_LESS(moving.GetOrg().x, destination.x, ());
  TEST_GREATER(moving.GetScale(), targetScale, ());
  TEST_LESS(moving.GetScale(), start.GetScale(), ());

  auto const & settled = Process(At(time, duration + 0.001));
  TEST(settled.GetOrg().EqualDxDy(destination, 1e-9), ());
  TEST_ALMOST_EQUAL_ABS(settled.GetScale(), targetScale, 1e-10, ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, NearbyCenterMovesWithoutZoomingOut)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  // Four lengths of the shorter viewport side.
  double const duration = GetMoveTime(4.0, 0.0 /* zoomLevels */);
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(4.0 * kViewportSide, 0.0));
  auto const time = AddEvent<df::SetCenterEvent>(destination, df::kDoNotChangeZoom, true, false);

  auto const & moving = Process(At(time, duration * 0.5));
  double const moved = moving.GetOrg().Length(start.GetOrg()) / destination.Length(start.GetOrg());
  TEST(moved > 0.5 && moved < 1.0, (moved));
  TEST_EQUAL(moving.GetScale(), start.GetScale(), ());
  TEST(Process(At(time, duration + 0.001)).GetOrg().EqualDxDy(destination, 1e-9), ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, NearbyZoomInPansFirstAndZoomsThroughout)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(kViewportSide, 0.0));
  int const zoom = GetZoom(start, 3);
  double const zoomLevels = GetZoomLevels(start, zoom);
  double const duration = GetMoveTime(1.0, zoomLevels);
  auto const time = AddEvent<df::SetCenterEvent>(destination, zoom, true, false);

  // The pan eases out: more than a half of the way on the screen in a third of the time.
  auto const & moving = Process(At(time, duration * 0.3));
  double const offset = moving.GtoP(destination).Length(moving.PixelRect().Center());
  TEST_LESS(offset, kViewportSide * 0.5, ());
  double const zoomed = std::log2(start.GetScale() / moving.GetScale());
  TEST(zoomed > 0.0 && zoomed < zoomLevels, (zoomed));
  auto const & settled = Process(At(time, duration + 0.001));
  TEST(!m_animations.IsCameraMoving(), ());
  TEST(settled.GetOrg().EqualDxDy(destination, 1e-9), ());
  TEST_ALMOST_EQUAL_ABS(settled.GetScale(), df::GetScreenScale(zoom), 1e-12, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, DistantCenterFliesAtTheFlightZoom)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(8000.0, 0.0));
  // The map zooms out to show the distance as kFlyViewports lengths of the shorter viewport side and stays at this
  // scale until kFlyZoomStart of the flight.
  double const flightScale = GetFlightScale(start, 8000.0);
  double const duration = GetFlyTime(std::log2(flightScale / start.GetScale()));
  auto const time = AddEvent<df::SetCenterEvent>(destination, df::kDoNotChangeZoom, true, false);

  double previousOffset = 8000.0;
  for (double t = 0.001; t < duration; t += 0.05)
  {
    auto const & screen = Process(At(time, t));
    if (t >= df::kFlyZoomOutTimeSec && t < duration * df::kFlyZoomStart)
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
  double const flightScale = GetFlightScale(start, 8000.0);
  double const endScale = df::GetScreenScale(zoom);
  double const duration = GetFlyTime(std::log2(flightScale / endScale));
  auto const time = AddEvent<df::SetCenterEvent>(destination, zoom, true, false);

  // On arrival, the map zooms in while the destination keeps coming closer.
  double previousScale = flightScale;
  double previousOffset = df::kFlyViewports * kViewportSide;
  for (double t = duration * df::kFlyZoomStart + 0.05; t < duration; t += 0.05)
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

namespace
{
// Checks that a flight to destination at zoom continues after a change before the arrival zoom and lands at the
// place given by the change, at the scale endScale, on the original deadline.
template <typename Change>
void CheckFlightKeepsGoing(CameraAnimationFixture & fixture, Change && change, double scaleFactor)
{
  ScreenBase const start = fixture.m_stream.GetCurrentScreen();
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(8000.0, 0.0));
  int const zoom = static_cast<int>(df::GetZoomLevel(start.GetScale())) + 6;
  double const flightScale = GetFlightScale(start, 8000.0);
  double const duration = GetFlyTime(std::log2(flightScale / df::GetScreenScale(zoom)));
  double const zoomStart = duration * df::kFlyZoomStart;
  auto const time = fixture.AddEvent<df::SetCenterEvent>(destination, zoom, true, true /* trackVisibleViewport */);
  ScreenBase const before = fixture.Process(CameraAnimationFixture::At(time, zoomStart * 0.5));

  change(destination, df::GetScreenScale(zoom) * scaleFactor);
  auto const & after = fixture.Process(CameraAnimationFixture::At(time, zoomStart * 0.5));
  TEST(after.GetOrg().EqualDxDy(before.GetOrg(), 1e-9), (after.GetOrg(), before.GetOrg()));
  TEST_ALMOST_EQUAL_ABS(after.GetScale(), before.GetScale(), 1e-12, ());

  TEST_ALMOST_EQUAL_ABS(fixture.Process(CameraAnimationFixture::At(time, zoomStart * 0.9)).GetScale(), flightScale,
                        1e-12, ());
  double previousScale = flightScale;
  for (double t = zoomStart + 0.05; t < duration; t += 0.05)
  {
    double const scale = fixture.Process(CameraAnimationFixture::At(time, t)).GetScale();
    TEST_LESS(scale, previousScale, (t));
    previousScale = scale;
  }
  auto const & settled = fixture.Process(CameraAnimationFixture::At(time, duration + 0.001));
  TEST(!fixture.m_animations.IsCameraMoving(), ());
  TEST_ALMOST_EQUAL_ABS(settled.GetScale(), df::GetScreenScale(zoom) * scaleFactor, 1e-12, ());
}
}  // namespace

UNIT_CLASS_TEST(CameraAnimationFixture, PlacePageDuringAFlightKeepsFlying)
{
  m2::RectD const viewport(0.0, 0.0, 800.0, 420.0);
  m2::PointD destination;
  CheckFlightKeepsGoing(*this, [&](m2::PointD const & point, double)
  {
    destination = point;
    AddEvent<df::SetVisibleViewportEvent>(viewport);
  }, 1.0);
  TEST(m_stream.GetCurrentScreen().GtoP(destination).EqualDxDy(viewport.Center(), 1e-6), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, SelectionOutlineDuringAFlightKeepsFlying)
{
  m2::PointD destination;
  CheckFlightKeepsGoing(*this, [&](m2::PointD const & point, double scale)
  {
    destination = point;
    m2::RectD rect(point, point);
    rect.Inflate(400.0 * scale, 300.0 * scale);
    AddEvent<df::SetRectEvent>(rect, false /* rotate */, df::kDoNotChangeZoom, true /* isAnim */,
                               false /* useVisibleViewport */);
  }, 0.5);
  TEST(m_stream.GetCurrentScreen().GetOrg().EqualDxDy(destination, 1e-9), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, DeepZoomWithAShortMoveFlies)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  // The destination is a little farther than the start scale shows, and the end scale is much smaller.
  double const pixels = (df::kFlyViewports + 1.5) * kViewportSide;
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(pixels, 0.0));
  int const zoom = static_cast<int>(df::GetZoomLevel(start.GetScale())) + 9;
  double const duration = GetFlyTime(std::log2(GetFlightScale(start, pixels) / df::GetScreenScale(zoom)));
  auto const time = AddEvent<df::SetCenterEvent>(destination, zoom, true, false);
  auto const & flying = Process(At(time, (df::kFlyZoomOutTimeSec + duration * df::kFlyZoomStart) / 2.0));
  TEST_ALMOST_EQUAL_ABS(flying.GetScale(), GetFlightScale(start, pixels), 1e-12, ());
  TEST_GREATER(flying.GetScale(), start.GetScale(), ());
  auto const & settled = Process(At(time, duration + 0.001));
  TEST(settled.GetOrg().EqualDxDy(destination, 1e-9), ());
  TEST_EQUAL(settled.GetScale(), df::GetScreenScale(zoom), ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FollowFliesToADistantPosition)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const anchor = start.PixelRect().Center();
  // A little farther than kFlyViewports, where a linear follow would take less than kMaxAnimationTimeSec.
  double const pixels = (df::kFlyViewports + 2.0) * kViewportSide;
  auto const position = start.PtoG(anchor + m2::PointD(pixels, 0.0));
  int const zoom = GetZoom(start, 1);
  auto const time = AddEvent<df::FollowAndRotateEvent>(position, anchor, 0.3, zoom);

  // After the zoom-out, the map shows the distance as kFlyViewports.
  double const duration = GetFlyTime(std::log2(GetFlightScale(start, pixels) / df::GetScreenScale(zoom)));
  auto const & flying = Process(At(time, (df::kFlyZoomOutTimeSec + duration * df::kFlyZoomStart) / 2.0));
  TEST_ALMOST_EQUAL_ABS(flying.GetScale(), GetFlightScale(start, pixels), 1e-12, ());
  TEST_GREATER(flying.GetScale(), start.GetScale(), ());

  auto const & settled = Process(At(time, duration + 0.001));
  TEST(!m_animations.IsCameraMoving(), ());
  TEST(settled.GtoP(position).EqualDxDy(anchor, 1e-6), (settled.GtoP(position), anchor));
  TEST_EQUAL(settled.GetScale(), df::GetScreenScale(zoom), ());
  TEST_ALMOST_EQUAL_ABS(settled.GetAngle(), -0.3, 1e-10, ());
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
  TEST_LESS(first.GtoP(start.GetOrg()).Length(first.PixelRect().Center()), kViewportSide / 2.0, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FlightTurnsTheShortestWay)
{
  // The heading 5.5 gives the screen angle -5.5: a quarter turn from the start angle 5.5 by the shortest way.
  Process(AddEvent<df::RotateEvent>(5.5, false));
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const anchor = start.PixelRect().Center();
  auto const position = start.PtoG(anchor + m2::PointD(8000.0, 0.0));
  auto const time = AddEvent<df::FollowAndRotateEvent>(position, anchor, 5.5, df::kDoNotChangeZoom);
  double const turn = ang::GetShortestDistance(5.5, -5.5);
  for (double t = 0.05; t < df::kMinFlyTimeSec; t += 0.05)
  {
    double const turned = Process(At(time, t)).GetAngle() - 5.5;
    TEST(turned > 0.0 && turned < turn, (t, turned, turn));
  }
}

UNIT_CLASS_TEST(CameraAnimationFixture, ZoomOutNeverMovesAwayFromTheDestination)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(kViewportSide, 0.0));
  double const distance = start.GetOrg().Length(destination);
  int const zoom = GetZoom(start, -3);
  double const duration = GetMoveTime(1.0, GetZoomLevels(start, zoom));
  auto const time = AddEvent<df::SetCenterEvent>(destination, zoom, true, false);
  for (double t = 0.0; t < duration; t += 0.03)
  {
    auto const & screen = Process(At(time, t));
    TEST_LESS_OR_EQUAL(screen.GetOrg().Length(destination), distance * (1.0 + 1e-9), (t));
  }
  Process(At(time, duration + 0.001));
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, ZoomOutThatShowsTheDistanceMovesWithoutFlying)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  // Twenty lengths of the shorter viewport side are less than three at the end scale.
  auto const destination = start.PtoG(start.PixelRect().Center() + m2::PointD(20.0 * kViewportSide, 0.0));
  double const distance = start.GetOrg().Length(destination);
  int const zoom = GetZoom(start, -3);
  double const duration = GetMoveTime(20.0, GetZoomLevels(start, zoom));
  auto const time = AddEvent<df::SetCenterEvent>(destination, zoom, true, false);
  for (double t = 0.0; t < duration; t += 0.05)
  {
    auto const & screen = Process(At(time, t));
    TEST_LESS_OR_EQUAL(screen.GetScale(), df::GetScreenScale(zoom) * (1.0 + 1e-9), (t));
    TEST_LESS_OR_EQUAL(screen.GetOrg().Length(destination), distance * (1.0 + 1e-9), (t));
  }
  auto const & settled = Process(At(time, duration + 0.001));
  TEST(!m_animations.IsCameraMoving(), ());
  TEST(settled.GetOrg().EqualDxDy(destination, 1e-9), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, DeepZoomTakesTheLongestMoveTime)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  // Six zoom levels would take longer.
  int const zoom = GetZoom(start, 6);
  double const endScale = df::GetScreenScale(zoom);
  double const duration = df::kMaxMoveTimeSec;
  TEST_EQUAL(GetMoveTime(0.0, GetZoomLevels(start, zoom)), duration, ());
  auto const time = AddEvent<df::SetCenterEvent>(start.GetOrg(), zoom, true, false);
  auto const & moving = Process(At(time, duration * 0.5));
  TEST(m_animations.IsCameraMoving(), ());
  // The scale eases out.
  TEST_LESS(moving.GetScale(), (start.GetScale() + endScale) * 0.5, ());
  TEST_GREATER(moving.GetScale(), endScale, ());
  TEST_ALMOST_EQUAL_ABS(Process(At(time, duration + 0.001)).GetScale(), endScale, 1e-12, ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FollowZoomAnimatesWithinTheTimeLimit)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const anchor = start.PixelRect().Center();
  int const zoom = static_cast<int>(df::GetZoomLevel(start.GetScale())) + 6;
  auto const time = AddEvent<df::FollowAndRotateEvent>(start.GetOrg(), anchor, 0.0, zoom);
  auto const & moving = Process(At(time, df::kMaxAnimationTimeSec * 0.5));
  TEST(m_animations.IsCameraMoving(), ());
  TEST_GREATER(moving.GetScale(), df::GetScreenScale(zoom), ());
  TEST_ALMOST_EQUAL_ABS(Process(At(time, df::kMaxAnimationTimeSec + 0.001)).GetScale(), df::GetScreenScale(zoom), 1e-12,
                        ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, QuarterTurnTakesAHalfOfTheHalfTurnTime)
{
  double const duration = std::min(df::kHalfTurnTimeSec / 2.0, df::kMaxAnimationTimeSec);
  auto const time = AddEvent<df::RotateEvent>(math::pi2, true);
  TEST_ALMOST_EQUAL_ABS(Process(At(time, duration * 0.5)).GetAngle(), math::pi4, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(Process(At(time, duration + 0.001)).GetAngle(), math::pi2, 1e-9, ());
  TEST(!m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, RotationKeepsTheVisibleViewportCenter)
{
  // A place page on the right.
  m2::RectD const viewport(0.0, 0.0, 500.0, kViewportSide);
  Process(AddEvent<df::SetVisibleViewportEvent>(viewport));
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const point = start.PtoG(start.P3dtoP(viewport.Center()));

  auto const time = AddEvent<df::RotateEvent>(math::pi, true);
  for (double t = 0.05; t < df::kHalfTurnTimeSec; t += 0.05)
  {
    auto const & screen = Process(At(time, t));
    TEST(screen.GtoP(point).EqualDxDy(viewport.Center(), 1e-6), (t, screen.GtoP(point)));
  }
  TEST_ALMOST_EQUAL_ABS(ang::AngleIn2PI(Process(At(time, df::kHalfTurnTimeSec + 0.001)).GetAngle()), math::pi, 1e-9,
                        ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, HeadingUpdateEndsTheZoomOnTime)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const center = start.PixelRect().Center();
  auto const zoomTime = AddEvent<df::ScaleEvent>(2.0, center, true);
  // The heading update turns for longer than the zoom takes and keeps the zoom.
  AddEvent<df::FollowAndRotateEvent>(start.GetOrg(), center, math::pi2, df::kDoNotChangeZoom);
  TEST_GREATER(df::kHalfTurnTimeSec / 2.0, GetZoomTime(2.0), ());

  // One more heading update in the middle of the zoom.
  Process(At(zoomTime, GetZoomTime(2.0) / 2.0));
  AddEvent<df::FollowAndRotateEvent>(start.GetOrg(), center, math::pi2, df::kDoNotChangeZoom);

  auto const & zoomed = Process(At(zoomTime, GetZoomTime(2.0) + 0.001));
  TEST_ALMOST_EQUAL_ABS(zoomed.GetScale(), start.GetScale() / 2.0, 1e-12, ());
  TEST(m_animations.IsCameraMoving(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, RecenterEndsTheZoomOnTime)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  auto const center = start.PixelRect().Center();
  auto const zoomTime = AddEvent<df::ScaleEvent>(2.0, center, true);
  // A recenter without the arrow keeps the zoom and pans for longer than the zoom takes.
  Process(At(zoomTime, GetZoomTime(2.0) / 2.0));
  auto const destination = start.PtoG(center + m2::PointD(3.0 * kViewportSide, 0.0));
  AddEvent<df::SetCenterEvent>(destination, df::kDoNotChangeZoom, true, false);
  TEST_GREATER(GetMoveTime(3.0, 0.0 /* zoomLevels */), GetZoomTime(2.0), ());

  auto const & zoomed = Process(At(zoomTime, GetZoomTime(2.0) + 0.001));
  TEST_ALMOST_EQUAL_ABS(zoomed.GetScale(), start.GetScale() / 2.0, 1e-12, ());
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
  double const arrival = std::max(df::kShortMoveTimeSec, df::kHalfTurnTimeSec * 0.4 / math::pi);
  auto const & moving = Process(At(requestedAt, arrival / 2.0));
  auto const movingArrow = m_animations.GetArrowPose();
  TEST(movingArrow.has_value(), ());
  TEST(moving.GtoP(movingArrow->m_position).EqualDxDy(anchor, 1e-6), (moving.GtoP(movingArrow->m_position), anchor));
  TEST(!movingArrow->m_position.EqualDxDy(position, 1e-9), ());

  // The position reaches the anchor with the arrow when the short move and the turn end.
  auto const & arrived = Process(At(requestedAt, arrival + 0.001));
  TEST(arrived.GtoP(position).EqualDxDy(anchor, 1e-6), (arrived.GtoP(position), anchor));
  TEST_ALMOST_EQUAL_ABS(arrived.GetAngle(), -0.4, 1e-10, ());
  auto const arrowPose = m_animations.GetArrowPose();
  TEST(arrowPose.has_value(), ());
  TEST(arrowPose->m_position.EqualDxDy(position, 1e-9), (arrowPose->m_position, position));
  TEST(m_animations.IsCameraMoving(), ());
  TEST_GREATER(arrived.GetScale(), endScale, ());

  auto const & settled = Process(At(requestedAt, df::kAutoZoomTimeSecPerScaleRatio * 1.25 + 0.001));
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
  for (double t = 0.02; t < df::kHalfTurnTimeSec / 2.0; t += 0.04)
  {
    auto const & screen = Process(At(time, t));
    auto const arrow = m_animations.GetArrowPose();
    TEST(arrow.has_value(), (t));
    TEST(screen.GtoP(arrow->m_position).EqualDxDy(anchor, 1e-6), (t, screen.GtoP(arrow->m_position)));
    // The arrow turns with the map.
    TEST_ALMOST_EQUAL_ABS(ang::AngleIn2PI(arrow->m_azimuth + screen.GetAngle()), 0.0, 1e-9, (t));
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
  Process(At(time, df::kShortMoveTimeSec + 0.001));
  auto const settled = m_animations.GetArrowPose();
  TEST(settled.has_value(), ());
  TEST_EQUAL(settled->m_position, nextTarget.m_position, ());
  TEST_EQUAL(settled->m_azimuth, nextTarget.m_azimuth, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, PositionUpdateKeepsTheArrowAtTheCenter)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  m_animations.HoldArrowPose({start.GetOrg(), 0.0});
  // Two lengths of the shorter viewport side.
  auto const position = start.PtoG(start.PixelRect().Center() + m2::PointD(2.0 * kViewportSide, 0.0));
  auto const time =
      AddEvent<df::SetCenterEvent>(position, df::kDoNotChangeZoom, true, false, df::ArrowPose{position, 0.0});

  auto const & moving = Process(At(time, GetMoveTime(2.0, 0.0 /* zoomLevels */) * 0.35));
  TEST(m_animations.IsCameraMoving(), ());
  auto const arrow = m_animations.GetArrowPose();
  TEST(arrow.has_value(), ());
  TEST(!arrow->m_position.EqualDxDy(position, 1e-9), ());
  TEST(moving.GtoP(arrow->m_position).EqualDxDy(moving.PixelRect().Center(), 1e-6), (moving.GtoP(arrow->m_position)));
}

UNIT_CLASS_TEST(CameraAnimationFixture, LateStepStartsANewMove)
{
  ScreenBase const start = m_stream.GetCurrentScreen();
  ScreenBase first = start;
  first.Move(80.0, 0.0);
  ScreenBase second = first;
  second.Move(80.0, 0.0);
  auto const time = df::AnimationClock::now();
  auto const animation = df::GetScreenAnimation(df::MapAnimation::Type::Pan, start, first, time);
  TEST(animation.has_value(), ());

  // A step before the first frame joins the running move.
  auto early = *animation;
  TEST(early.Retarget(second, time), ());
  // In the middle of the move, the rest of it is too short for one more step.
  auto late = *animation;
  auto const middle = At(time, df::kShortMoveTimeSec / 2.0);
  late.Advance(middle);
  TEST(!late.Retarget(second, middle), ());
  // Neither is the rest of the time after a stalled frame.
  auto stalled = *animation;
  TEST(!stalled.Retarget(second, middle), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FlingNeedsAMovingFinger)
{
  df::KineticScroller scroller;
  ScreenBase screen = m_stream.GetCurrentScreen();
  auto const start = df::AnimationClock::now();
  scroller.Init(screen, start);
  // A fast drag, then the finger stays still before the release. The average speed is still above the fling one.
  screen.SetOrg(screen.GetOrg() + m2::PointD(300.0 * screen.GetScale(), 0));
  scroller.Update(screen, start + 20ms);
  TEST_GREATER(300.0 / 0.32, df::kInstantVelocityThreshold, ());
  TEST(!scroller.CreateKineticAnimation(screen, start + 320ms).has_value(), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FlingGlidesWithADecayingSpeed)
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

  // The release speed of 500 px/s carries the map kKineticAcceleration of it, and the speed decays exponentially.
  double const fullDistance = 500.0 * df::kKineticAcceleration;
  ScreenBase sample = screen;
  fling->Advance(release + 1ms);
  fling->Apply(sample);
  double const initialSpeed = sample.GetOrg().Length(screen.GetOrg()) / scale / 0.001;
  double const expectedSpeed =
      fullDistance * df::kKineticFadeoff / df::kKineticDuration / (1.0 - std::exp(-df::kKineticFadeoff));
  TEST_ALMOST_EQUAL_ABS(initialSpeed, expectedSpeed, expectedSpeed * 0.01, ());

  fling->Advance(At(release, df::kKineticDuration - 0.001));
  TEST(!fling->IsFinished(), ());
  fling->Advance(At(release, df::kKineticDuration + 0.001));
  TEST(fling->IsFinished(), ());
  fling->Apply(sample);
  double const distance = sample.GetOrg().Length(screen.GetOrg()) / scale;
  TEST_ALMOST_EQUAL_ABS(distance, fullDistance, 1e-6, ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FlingStopsAtThePole)
{
  // The top of the screen is 75 px from the north pole.
  auto const & world = df::GetWorldRect();
  ScreenBase screen = m_stream.GetCurrentScreen();
  double const scale = screen.GetScale();
  screen.SetOrg({0.0, world.maxY() - (screen.PixelRect().SizeY() / 2.0 + 75.0) * scale});
  double const room = world.maxY() - screen.ClipRect().maxY();
  TEST_GREATER(room, 0.0, ());

  // A fast diagonal swipe to the north-east.
  df::KineticScroller scroller;
  auto const start = df::AnimationClock::now();
  scroller.Init(screen, start);
  ScreenBase released = screen;
  released.SetOrg(screen.GetOrg() + m2::PointD(30.0, 30.0) * scale);
  auto const release = start + 10ms;
  scroller.Update(released, release);
  auto fling = scroller.CreateKineticAnimation(released, release);
  TEST(fling.has_value(), ());

  fling->Advance(At(release, df::kKineticDuration + 0.001));
  ScreenBase settled = released;
  fling->Apply(settled);
  TEST_LESS_OR_EQUAL(settled.ClipRect().maxY(), world.maxY(), ());
  TEST_GREATER(settled.GetOrg().y, released.GetOrg().y, ());
  // The horizontal travel isn't cut by the pole.
  TEST_GREATER(settled.GetOrg().x - released.GetOrg().x, 2.0 * (settled.GetOrg().y - released.GetOrg().y), ());
}

UNIT_CLASS_TEST(CameraAnimationFixture, FreshSwipeImmediatelyAfterPinchCanFling)
{
  auto const pinch = [&](df::TouchEvent::ETouchType type, float secondX)
  {
    auto event = make_unique_dp<df::TouchEvent>();
    event->SetTouchType(type);
    event->SetFirstTouch({m2::PointF(200, 300), 1});
    event->SetSecondTouch({m2::PointF(secondX, 300), 2});
    if (type != df::TouchEvent::TOUCH_MOVE)
      event->SetFirstMaskedPointer(1);  // Only the second finger is added or lifted.
    m_stream.AddEvent(std::move(event));
  };

  for (bool const freshSwipe : {false, true})
  {
    double const initialScale = m_stream.GetCurrentScreen().GetScale();
    Finger(df::TouchEvent::TOUCH_DOWN, 200, 300);
    pinch(df::TouchEvent::TOUCH_DOWN, 500);
    pinch(df::TouchEvent::TOUCH_MOVE, 540);
    pinch(df::TouchEvent::TOUCH_MOVE, 600);
    pinch(df::TouchEvent::TOUCH_UP, 600);
    ScreenBase const zoomed = Process(df::AnimationClock::now());
    TEST_LESS(zoomed.GetScale(), initialScale, ());

    // The finger left from the pinch doesn't fling, a new one does.
    if (freshSwipe)
    {
      Finger(df::TouchEvent::TOUCH_UP, 200, 300);
      Finger(df::TouchEvent::TOUCH_DOWN, 200, 300);
    }
    for (float x : {240.f, 280.f, 360.f, 440.f})
      Finger(df::TouchEvent::TOUCH_MOVE, x, 300);
    Finger(df::TouchEvent::TOUCH_UP, 440, 300);
    auto const & dragged = Process(df::AnimationClock::now());
    TEST(dragged.GetOrg() != zoomed.GetOrg(), (freshSwipe));
    TEST_EQUAL(m_animations.HasMapAnimation(df::MapAnimation::Type::Fling), freshSwipe, ());
    m_animations.CancelMapAnimation();
  }
}

UNIT_TEST(MoveDurationGrowsWithTheDistance)
{
  m2::RectD const viewport(0.0, 0.0, 800.0, kViewportSide);
  TEST_EQUAL(df::GetMoveDuration(0.0, viewport, 1.0 /* scale */), 0.0, ());
  double previous = df::kShortMoveTimeSec;
  for (double pixels = 1.0; pixels < 5.0 * kViewportSide; pixels += 10.0)
  {
    double const duration = df::GetMoveDuration(pixels, viewport, 1.0 /* scale */);
    TEST_GREATER_OR_EQUAL(duration, previous, (pixels));
    previous = duration;
  }
  TEST_ALMOST_EQUAL_ABS(df::GetMoveDuration(5.0 * kViewportSide, viewport, 1.0 /* scale */),
                        std::max(df::kShortMoveTimeSec, 5.0 / df::kMoveViewportsPerSec), 1e-12, ());
}

UNIT_TEST(OverlayFadeReachesEndpointAfterStalledFrame)
{
  df::ShowHideAnimation fade(false, df::kOverlayFadeTimeSec);
  fade.AnimateVisibility(true);
  df::InterpolationHolder::Instance().Advance(
      CameraAnimationFixture::At(df::AnimationClock::now(), df::kOverlayFadeTimeSec + 0.05));
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
