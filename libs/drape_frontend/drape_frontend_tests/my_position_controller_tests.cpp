#include "testing/testing.hpp"

#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/drape_notifier.hpp"
#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "drape/drape_routine.hpp"

#include "geometry/mercator.hpp"
#include "geometry/screenbase.hpp"

#include "base/scope_guard.hpp"

#include <chrono>
#include <utility>
#include <vector>

namespace my_position_controller_tests
{
using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, StopFollowModesAndFirstFix)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-100, -100, 100, 100)));
  location::GpsInfo fix;
  fix.m_latitude = 47;
  fix.m_longitude = 8;
  fix.m_horizontalAccuracy = 5;
  fix.m_timestamp = 1;

  df::AnimationSystem animations;
  for (auto mode : {location::Follow, location::FollowAndRotate, location::NotFollow})
  {
    df::MyPositionController controller(
        {mode, 0.0 /* timeInBackground */, {}, false /* isRoutingActive */, false /* isAutozoomEnabled */, {}}, nullptr,
        animations);
    controller.OnUpdateScreen(screen);
    controller.OnLocationUpdate(fix, false /* isNavigable */, screen);
    TEST_EQUAL(controller.GetCurrentMode(), mode, ());
    controller.StopLocationFollow();
    TEST_EQUAL(controller.GetCurrentMode(), location::NotFollow, ());
  }

  df::MyPositionController pending({location::Follow,
                                    0.0 /* timeInBackground */,
                                    {},
                                    false /* isRoutingActive */,
                                    false /* isAutozoomEnabled */,
                                    {}},
                                   nullptr, animations);
  pending.OnUpdateScreen(screen);
  TEST_EQUAL(pending.GetCurrentMode(), location::PendingPosition, ());
  pending.StopLocationFollow();
  pending.OnLocationUpdate(fix, false /* isNavigable */, screen);
  TEST_EQUAL(pending.GetCurrentMode(), location::NotFollow, ());
}
namespace
{
class PositionListener : public df::MyPositionController::Listener
{
public:
  void PositionChanged(m2::PointD const &, bool) override {}
  void ChangeModelView(m2::PointD const &, int, std::optional<df::ArrowPose> const & target) override
  {
    m_arrowTarget = target;
  }
  void ChangeModelView(double, std::optional<df::ArrowPose> const &) override {}
  void ChangeModelView(m2::RectD const &, std::optional<df::ArrowPose> const &) override {}
  void ChangeModelView(m2::PointD const &, double, m2::PointD const &, int,
                       std::optional<df::ArrowPose> const &) override
  {}
  void ChangeModelView(double scale, m2::PointD const &, double, m2::PointD const &,
                       std::optional<df::ArrowPose> const &) override
  {
    m_autoZoomScales.push_back(scale);
  }

  std::optional<df::ArrowPose> m_arrowTarget;
  std::vector<double> m_autoZoomScales;
};
}  // namespace

UNIT_CLASS_TEST(VisualParamsFixture, FollowSnapsAndAnimatesArrowWithCamera)
{
  using namespace std::chrono_literals;
  df::AnimationSystem animations;
  df::UserEventStream stream(animations);
  stream.AddEvent(make_unique_dp<df::ResizeEvent>(800, 600));
  stream.AddEvent(
      make_unique_dp<df::SetRectEvent>(m2::RectD(-2.0, -2.0, 2.0, 2.0), false, df::kDoNotChangeZoom, false, false));
  bool changed, resized, active;
  ScreenBase const screen = stream.ProcessEvents(changed, resized, active, df::AnimationClock::now());

  PositionListener listener;
  df::MyPositionController controller({location::Follow, 0.0, {}, false, false, {}}, nullptr, animations);
  controller.SetListener(make_ref(&listener));
  controller.OnUpdateScreen(screen);
  location::GpsInfo fix;
  fix.m_latitude = 10.0;
  fix.m_longitude = 80.0;
  fix.m_horizontalAccuracy = 5.0;
  fix.m_timestamp = 1.0;
  controller.OnLocationUpdate(fix, false, screen);
  auto const position = controller.Position();
  animations.HoldArrowPose({position, 0.0});
  animations.AnimateArrowTo({position + m2::PointD(0.005, 0.0), 0.0}, df::AnimationClock::now(), 0.2);
  fix.m_longitude += 0.01;
  fix.m_timestamp += 1.0;
  controller.OnLocationUpdate(fix, false, screen);
  controller.UpdatePosition();
  TEST(listener.m_arrowTarget.has_value(), ());
  TEST(animations.GetArrowPose().has_value(), ());

  auto const follow = [&](std::chrono::milliseconds elapsed) -> ScreenBase const &
  {
    auto event =
        make_unique_dp<df::SetCenterEvent>(controller.Position(), df::kDoNotChangeZoom, listener.m_arrowTarget);
    auto const time = event->GetRequestTime();
    stream.AddEvent(std::move(event));
    return stream.ProcessEvents(changed, resized, active, time + elapsed);
  };
  auto const & settled = follow(0ms);
  TEST(settled.GetOrg().EqualDxDy(controller.Position(), 1e-6), ());
  TEST(!animations.GetArrowPose().has_value(), ());
  TEST(controller.GetDrawablePosition().EqualDxDy(controller.Position(), 1e-6), ());

  auto const & viewport = settled.PixelRect();
  auto const destination = settled.PtoG(viewport.Center() + m2::PointD(2.5 * viewport.SizeX(), 0.0));
  fix.m_longitude = mercator::XToLon(destination.x);
  fix.m_timestamp += 1.0;
  controller.OnLocationUpdate(fix, false, settled);
  controller.UpdatePosition();
  auto const & moving = follow(100ms);
  TEST(animations.IsCameraMoving(), ());
  TEST(moving.GetOrg().EqualDxDy(controller.GetDrawablePosition(), 1e-6),
       (moving.GetOrg(), controller.GetDrawablePosition()));
}

UNIT_CLASS_TEST(VisualParamsFixture, AutoZoomFiltersSmallChangesAndRetriesAfterShortDrag)
{
  using namespace std::chrono_literals;
  df::AnimationSystem animations;
  df::ThreadsCommutator commutator;
  df::DrapeNotifier notifier(make_ref(&commutator));
  PositionListener listener;
  df::MyPositionController controller({location::FollowAndRotate, 0.0, {}, true, true, {}}, make_ref(&notifier),
                                      animations);
  controller.SetListener(make_ref(&listener));
  dp::DrapeRoutine::Init();
  SCOPE_GUARD(stop, [] { dp::DrapeRoutine::Shutdown(); });

  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  controller.OnUpdateScreen(screen);
  location::GpsInfo fix;
  fix.m_latitude = 47.0;
  fix.m_longitude = 8.0;
  fix.m_horizontalAccuracy = 5.0;
  fix.m_timestamp = 1.0;
  fix.m_speed = 40.0 / 3.6;
  controller.OnLocationUpdate(fix, false, screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::FollowAndRotate, ());
  controller.ScaleStarted();
  controller.ScaleEnded();

  auto const now = df::AnimationClock::now();
  controller.UpdateViewportForLocation(now);
  TEST(listener.m_autoZoomScales.empty(), ());
  controller.UpdateViewportForLocation(now + 11s);
  TEST_EQUAL(listener.m_autoZoomScales.size(), 1, ());

  fix.m_timestamp += 1.0;
  fix.m_speed = 40.5 / 3.6;
  controller.OnLocationUpdate(fix, false, screen);
  controller.UpdateViewportForLocation(now + 12s);
  TEST_EQUAL(listener.m_autoZoomScales.size(), 1, ());

  fix.m_timestamp += 1.0;
  fix.m_speed = 50.0 / 3.6;
  controller.OnLocationUpdate(fix, false, screen);
  controller.UpdateViewportForLocation(now + 13s);
  TEST_EQUAL(listener.m_autoZoomScales.size(), 2, ());
  double const requestedScale = listener.m_autoZoomScales.back();

  screen.SetScale(listener.m_autoZoomScales.front());
  ScreenBase target = screen;
  target.SetScale(requestedScale);
  animations.StartMapAnimation(
      {df::MapAnimation::Type::Camera, screen, target, now + 13s, df::kAutoZoomAnimationTimeSec});
  controller.DragStarted(screen);
  animations.CancelMapAnimation();
  controller.UpdateViewportForLocation(now + 13s + 50ms);
  TEST_EQUAL(listener.m_autoZoomScales.size(), 2, ());
  controller.DragEnded({12.0, 0.0});
  TEST_EQUAL(controller.GetCurrentMode(), location::FollowAndRotate, ());
  controller.UpdateViewportForLocation(now + 13s + 100ms);
  TEST_EQUAL(listener.m_autoZoomScales.size(), 3, ());
  TEST_EQUAL(listener.m_autoZoomScales.back(), requestedScale, ());
}
}  // namespace my_position_controller_tests
