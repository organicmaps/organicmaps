#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/location.hpp"

#include "geometry/screenbase.hpp"

#include <optional>

namespace my_position_controller_tests
{
using df::kDoNotChangeZoom;

/// Remembers how the controller has asked to show the map.
class Listener : public df::MyPositionController::Listener
{
public:
  void PositionChanged(m2::PointD const &, bool) override {}
  void ChangeModelView(m2::PointD const &, int zoomLevel, df::TAnimationCreator const &) override
  {
    m_zoomLevel = zoomLevel;
  }
  void ChangeModelView(double, df::TAnimationCreator const &) override {}
  void ChangeModelView(m2::RectD const &, df::TAnimationCreator const &) override {}
  void ChangeModelView(m2::PointD const &, double, m2::PointD const &, int zoomLevel, df::Animation::TAction const &,
                       df::TAnimationCreator const &) override
  {
    m_zoomLevel = zoomLevel;
  }
  void ChangeModelView(double autoScale, m2::PointD const &, double, m2::PointD const &,
                       df::TAnimationCreator const &) override
  {
    m_autoScale = autoScale;
  }

  std::optional<int> m_zoomLevel;
  std::optional<double> m_autoScale;
};

class Map : public df::test_support::VisualParamsFixture
{
public:
  Map()
    : m_controller(df::MyPositionController::Params(location::Follow, 0.0 /* timeInBackground */, df::Hints(),
                                                    false /* isRoutingActive */, true /* isAutozoomEnabled */,
                                                    [](location::EMyPositionMode, bool) {}),
                   nullptr /* notifier */)
  {
    m_controller.SetListener(make_ref(&m_listener));
  }

  static ScreenBase Screen(int zoom)
  {
    ScreenBase screen;
    screen.OnSize(0, 0, 1080, 1920);
    // The middle of the zoom level, not to depend on the rounding.
    screen.SetFromParams(m2::PointD::Zero(), 0.0 /* angle */, df::GetScreenScale(zoom + 0.5));
    return screen;
  }

  void Locate(double accuracyM, double speedMps)
  {
    location::GpsInfo info;
    info.m_timestamp = ++m_timestamp;
    info.m_horizontalAccuracy = accuracyM;
    info.m_speed = speedMps;
    m_controller.OnLocationUpdate(info, m_controller.IsInRouting(), Screen(16));
  }

  /// The user moves the map away, leaves it at |zoom| and presses the button of the position.
  Listener const & MoveAwayAndPress(int zoom)
  {
    m_controller.Scrolled({100.0, 0.0});
    TEST_EQUAL(m_controller.GetCurrentMode(), location::NotFollow, ());
    return Press(zoom);
  }

  Listener const & Press(int zoom)
  {
    m_listener = {};
    m_controller.NextMode(Screen(zoom));
    return m_listener;
  }

  Listener m_listener;
  df::MyPositionController m_controller;
  double m_timestamp = 0.0;
};

UNIT_CLASS_TEST(Map, MyPositionController_ButtonBringsMapClose)
{
  Locate(15.0 /* accuracyM */, 0.0 /* speedMps */);
  TEST_EQUAL(m_controller.GetCurrentMode(), location::Follow, ());

  // A map left at the scale of a district comes to the scale of the streets.
  TEST_EQUAL(MoveAwayAndPress(13).m_zoomLevel, 16, ());
  TEST_EQUAL(m_controller.GetCurrentMode(), location::Follow, ());
  TEST_EQUAL(MoveAwayAndPress(5).m_zoomLevel, 16, ());
  // A closer map is left as it is.
  TEST_EQUAL(MoveAwayAndPress(16).m_zoomLevel, kDoNotChangeZoom, ());
  TEST_EQUAL(MoveAwayAndPress(18).m_zoomLevel, kDoNotChangeZoom, ());

  // The position is known within 20 km: there is nothing to look at closer than that.
  Locate(20000.0 /* accuracyM */, 0.0 /* speedMps */);
  TEST_EQUAL(MoveAwayAndPress(13).m_zoomLevel, kDoNotChangeZoom, ());
  auto const zoom = MoveAwayAndPress(5).m_zoomLevel;
  TEST(zoom && *zoom > 5 && *zoom < 13, (zoom));
}

UNIT_CLASS_TEST(Map, MyPositionController_ButtonBringsMapCloseInRouting)
{
  Locate(15.0 /* accuracyM */, 0.0 /* speedMps */);
  m_controller.ActivateRouting(17 /* zoomLevel */, true /* enableAutoZoom */, true /* isArrowGlued */);
  TEST_EQUAL(m_listener.m_zoomLevel, 17, ());

  // The car stands, there is no scale by the speed: the map comes to the scale the navigation has started with.
  auto view = MoveAwayAndPress(13);
  TEST_EQUAL(m_controller.GetCurrentMode(), location::FollowAndRotate, ());
  TEST_EQUAL(view.m_zoomLevel, 17, ());
  TEST(!view.m_autoScale, ());
  TEST_EQUAL(MoveAwayAndPress(18).m_zoomLevel, kDoNotChangeZoom, ());

  // The car drives: the scale by the speed at once, not 10 seconds later.
  Locate(15.0 /* accuracyM */, 10.0 /* speedMps */);
  view = MoveAwayAndPress(13);
  TEST_EQUAL(m_controller.GetCurrentMode(), location::FollowAndRotate, ());
  TEST(view.m_autoScale && *view.m_autoScale > 0.0, (view.m_autoScale));
  TEST(!view.m_zoomLevel, ());

  // The next press turns the map to the north and doesn't change the scale.
  view = Press(13);
  TEST_EQUAL(m_controller.GetCurrentMode(), location::Follow, ());
  TEST_EQUAL(view.m_zoomLevel, kDoNotChangeZoom, ());
}

UNIT_CLASS_TEST(Map, MyPositionController_ButtonBringsMapCloseInRoutingWithoutAutoZoom)
{
  Locate(15.0 /* accuracyM */, 10.0 /* speedMps */);
  m_controller.ActivateRouting(16 /* zoomLevel */, false /* enableAutoZoom */, true /* isArrowGlued */);

  auto const view = MoveAwayAndPress(13);
  TEST_EQUAL(view.m_zoomLevel, 16, ());
  TEST(!view.m_autoScale, ());
}
}  // namespace my_position_controller_tests
