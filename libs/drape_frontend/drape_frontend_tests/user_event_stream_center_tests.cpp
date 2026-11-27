#include "testing/testing.hpp"

#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "geometry/mercator.hpp"

namespace user_event_stream_center_tests
{
using df::test_support::VisualParamsFixture;

namespace
{
void ProcessEvents(df::UserEventStream & stream)
{
  bool modelViewChanged, viewportChanged, activeFrame;
  stream.ProcessEvents(modelViewChanged, viewportChanged, activeFrame);
}

// Advances the animations frame by frame, as the renderer does.
void FinishAnimations(df::UserEventStream & stream)
{
  double constexpr kFrameSec = 1.0 / 60.0;
  int constexpr kMaxFrames = 60 * 10;
  auto & animationSystem = df::AnimationSystem::Instance();
  for (int i = 0; i < kMaxFrames && animationSystem.HasAnimations(); ++i)
  {
    animationSystem.Advance(kFrameSec);
    ProcessEvents(stream);
  }
  TEST(!animationSystem.HasAnimations(), ());
}
}  // namespace

UNIT_CLASS_TEST(VisualParamsFixture, SetCenter_StaysInsideTheWorldNearThePole)
{
  // Centering a point near the pole in a low visible viewport would move the view beyond the pole: the view stops at
  // the pole, with the point centered horizontally.
  m2::RectD const kVisibleViewport(0.0, 1728.0, 1080.0, 1944.0);
  m2::PointD const target = mercator::FromLatLon(85.06, 12.5);

  for (bool isAnim : {false, true})
  {
    df::UserEventStream stream;
    stream.AddEvent(make_unique_dp<df::ResizeEvent>(1080, 2160));
    stream.AddEvent(make_unique_dp<df::SetVisibleViewportEvent>(kVisibleViewport));
    stream.AddEvent(make_unique_dp<df::SetCenterEvent>(mercator::FromLatLon(-40.5, 100.5), 10 /* zoom */,
                                                       false /* isAnim */, false /* trackVisibleViewport */,
                                                       nullptr /* parallelAnimCreator */));
    ProcessEvents(stream);

    stream.AddEvent(make_unique_dp<df::SetCenterEvent>(target, 15 /* zoom */, isAnim, false /* trackVisibleViewport */,
                                                       nullptr /* parallelAnimCreator */));
    ProcessEvents(stream);
    FinishAnimations(stream);

    ScreenBase const & screen = stream.GetCurrentScreen();
    TEST(df::CheckBorders(screen), (screen.ClipRect()));
    TEST_ALMOST_EQUAL_ABS(screen.ClipRect().maxY(), mercator::Bounds::kMaxY, 1e-6, ("isAnim", isAnim));
    double const centerX = screen.PtoG(screen.P3dtoP(kVisibleViewport.Center())).x;
    TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(target.x, centerX), centerX, 1e-6, ("isAnim", isAnim));
  }
}
}  // namespace user_event_stream_center_tests
