#include "testing/testing.hpp"

#include "routing/routing_tests/tools.hpp"

#include "routing/ruler_router.hpp"
#include "routing/track_router.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace track_voice_tests
{
using namespace routing;

std::string const kPhrase = "You have left the track.";
std::string const kApproach = "Proceed to the track.";
std::string const kRecalculating = "Recalculating route.";
std::string const kJson = R"({"you_have_left_the_track":"You have left the track.",
                             "proceed_to_the_track":"Proceed to the track.",
                             "route_recalculating":"Recalculating route."})";

m2::PointD Point(double x, double y)
{
  return {mercator::MetersToMercator(x), mercator::MetersToMercator(y)};
}

void WaitForBuild(std::future<RouterResultCode> & ready)
{
  TEST(ready.wait_for(std::chrono::seconds(30)) == std::future_status::ready, ("Route was not built."));
  TEST_EQUAL(ready.get(), RouterResultCode::NoError, ());
}

class TrackVoiceTest : public AsyncGuiThreadTestWithRoutingSession
{
public:
  std::vector<geometry::PointWithAltitude> m_track = {{Point(0, 0), 100}, {Point(1000, 0), 100}};

  void Build(double x = 0, double y = 0, bool voice = true, bool track = true)
  {
    RunOnGui([&]
    {
      if (!m_session)
        InitRoutingSession();
      m_hasFix = false;
      std::unique_ptr<IRouter> router;
      if (track)
      {
        auto data = std::make_shared<TrackFollowingData>();
        data->m_points = m_track;
        router = std::make_unique<TrackRouter>(VehicleType::Pedestrian, data);
      }
      else
      {
        router = std::make_unique<RulerRouter>();
      }
      m_session->SetRouter(std::move(router), nullptr);
      m_session->SetRoutingSettings(GetRoutingSettings(VehicleType::Pedestrian));
      m_session->SetLocaleWithJsonForTesting(kJson, "en");
      m_session->SetTurnNotificationsUnits(measurement_utils::Units::Metric);
      m_session->EnableTurnNotifications(voice);
    });
    Rebuild(x, y, true /* manual */);
  }

  void Rebuild(double x, double y, bool manual = false)
  {
    auto built = std::make_shared<std::promise<RouterResultCode>>();
    auto ready = built->get_future();
    RunOnGui([&, built]
    {
      auto onReady = [built](RoutesResult const &, RouterResultCode code) { built->set_value(code); };
      auto onError = [built](RouterResultCode code) { built->set_value(code); };
      if (manual)
      {
        m_session->SetRoutingCallbacks(onReady, nullptr, nullptr, onError);
        m_session->BuildRoute(Checkpoints(Point(x, y), m_track.back().GetPoint()), RouterDelegate::kNoTimeout);
      }
      else
      {
        m_session->RebuildRoute(Point(x, y), onReady, nullptr, onError, RouterDelegate::kNoTimeout,
                                SessionState::RouteRebuilding, true);
      }
    });
    WaitForBuild(ready);
    RunOnGui([&] { m_session->SetRoutingCallbacks(nullptr, nullptr, nullptr, nullptr); });
  }

  void Start()
  {
    RunOnGui([&] { TEST(m_session->EnableFollowMode(), ()); });
  }

  void Voice(bool enabled)
  {
    RunOnGui([&] { m_session->EnableTurnNotifications(enabled); });
  }

  // GUI-thread-only so a replay can assert its state before polling speech.
  void Feed(double x, double y)
  {
    auto const distance = m_hasFix ? std::hypot(x - m_x, y - m_y) : 0.0;
    m_time += std::max(1.0, distance / 2.0);
    location::GpsInfo info;
    auto const latLon = mercator::ToLatLon(Point(x, y));
    info.m_latitude = latLon.m_lat;
    info.m_longitude = latLon.m_lon;
    info.m_timestamp = m_time;
    info.m_horizontalAccuracy = 5.0;
    info.m_speed = 2.0;
    if (distance > 0.0)
    {
      auto bearing = math::RadToDeg(std::atan2(x - m_x, y - m_y));
      if (bearing < 0.0)
        bearing += 360.0;
      info.m_bearing = bearing;
    }
    m_session->OnLocationPositionChanged(info);
    m_x = x;
    m_y = y;
    m_hasFix = true;
  }

  void Depart(double x, double fromY = 0, double firstOffset = 20)
  {
    for (double y = firstOffset; y <= 240; y += 20)
      Feed(x, fromY + y);
    TEST(!m_session->IsOnRoute(), ());
  }

  void PrepareNotification(bool approach, bool voice = true)
  {
    Build(0, approach ? -100 : 0, voice);
    Start();
    if (!approach)
      RunOnGui([&]
      {
        Feed(200, 0);
        Depart(200);
      });
  }

  std::vector<std::string> Notifications()
  {
    std::vector<std::string> result;
    RunOnGui([&] { m_session->GenerateNotifications(result, false); });
    return result;
  }

private:
  double m_time = 100.0;
  double m_x = 0.0;
  double m_y = 0.0;
  bool m_hasFix = false;
};

UNIT_CLASS_TEST(TrackVoiceTest, DepartureAtTrackBoundary)
{
  for (double approachY : {0.0, -100.0})
  {
    Build(0, approachY);
    Start();
    RunOnGui([&]
    {
      Feed(0, 0);
      TEST(m_session->IsOnRoute(), ());
      Depart(0);
    });
    TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
    TEST(Notifications().empty(), ());
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, ApproachMustReachTrack)
{
  m_track.front() = {Point(200, 0), 100};
  Build(200, -100);
  Start();
  TEST_EQUAL(Notifications(), std::vector<std::string>({kApproach}), ());
  RunOnGui([&]
  {
    Feed(200, -100);
    for (double x = 220; x <= 440; x += 20)
      Feed(x, -100);
    TEST(!m_session->IsOnRoute(), ());
  });
  TEST(Notifications().empty(), ());
  Rebuild(440, -100);
  RunOnGui([&]
  {
    Feed(440, -100);
    for (double x = 460; x <= 680; x += 20)
      Feed(x, -100);
    TEST(!m_session->IsOnRoute(), ());
  });
  TEST(Notifications().empty(), ());
  RunOnGui([&]
  {
    // Reach this build's approach endpoint before advancing along the source track.
    Feed(200, 0);
    TEST(m_session->IsOnRoute(), ());
    Feed(700, 0);
    TEST(m_session->IsOnRoute(), ());
    Depart(700);
  });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, InitialApproachIsSpokenOnce)
{
  Build(0, -100);
  TEST(Notifications().empty(), ());
  Start();
  TEST_EQUAL(Notifications(), std::vector<std::string>({kApproach}), ());
  TEST(Notifications().empty(), ());
  RunOnGui([&]
  {
    Feed(0, -100);
    Feed(0, -80);
  });
  Start();
  TEST(Notifications().empty(), ());
  Rebuild(0, -80);
  Start();
  TEST(Notifications().empty(), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, StartingAtTrackArmsDeparture)
{
  for (double approachY : {0.0, -10.0, -100.0})
  {
    for (bool const voiceAtStart : {false, true})
    {
      Build(0, approachY, voiceAtStart);
      if (approachY == -100.0)
        RunOnGui([&] { Feed(0, 0); });
      Start();
      TEST(Notifications().empty(), (approachY, voiceAtStart));
      Voice(true);
      // The first new fix is already outside the corridor, so Start must have armed the warning.
      RunOnGui([&] { Depart(0, 0 /* fromY */, 40 /* firstOffset */); });
      TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), (approachY, voiceAtStart));
      TEST(Notifications().empty(), (approachY, voiceAtStart));
    }
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, ReachingTrackDropsPendingApproach)
{
  Build(0, -100);
  Start();
  RunOnGui([&] { Feed(0, 0); });
  TEST(Notifications().empty(), ());
  RunOnGui([&] { Depart(0); });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, MuteDiscardsTrackNotifications)
{
  for (bool const approach : {false, true})
  {
    for (auto const & [voiceAtEvent, pollWhileMuted] :
         {std::pair{false, false}, {false, true}, {true, false}, {true, true}})
    {
      PrepareNotification(approach, voiceAtEvent);
      if (voiceAtEvent)
        Voice(false);
      if (pollWhileMuted)
        TEST(Notifications().empty(), (approach, voiceAtEvent));
      Voice(true);
      TEST(Notifications().empty(), (approach, voiceAtEvent));
      if (approach)
      {
        Start();
        TEST(Notifications().empty(), ());
      }
    }
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, RejoinDropsPendingDeparture)
{
  Build();
  Start();
  RunOnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  RunOnGui([&]
  {
    Feed(400, 0);
    TEST(m_session->IsOnRoute(), ());
  });
  TEST(Notifications().empty(), ());
  RunOnGui([&] { Depart(400); });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, RebuildDoesNotRearmDeparture)
{
  Build();
  Start();
  RunOnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  Rebuild(200, 240);
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
  RunOnGui([&]
  {
    Feed(200, 240);
    for (double x = 220; x <= 440; x += 20)
      Feed(x, 240);
    TEST(!m_session->IsOnRoute(), ());
  });
  TEST(Notifications().empty(), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, FailedRebuildDoesNotRepeatDeparture)
{
  Build();
  Start();
  RunOnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
  RunOnGui([&]
  {
    m_session->SetState(SessionState::RouteRebuilding);
    m_session->AssignRouteForTesting(Route{}, RouterResultCode::InternalError);
    TEST(m_session->IsOnRoute(), ());
    Depart(200, 240);
  });
  TEST(Notifications().empty(), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, OnTrackRebuildKeepsConfirmedReach)
{
  Build();
  Start();
  RunOnGui([&]
  {
    Feed(200, 0);
    TEST(m_session->IsOnRoute(), ());
  });
  Rebuild(200, 0);
  RunOnGui([&] { Depart(200, 0 /* fromY */, 40 /* firstOffset */); });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, PreviewReachArmsDepartureOnStart)
{
  for (bool const rebuild : {false, true})
  {
    Build(0, -100);
    RunOnGui([&] { Feed(200, 0); });
    Start();
    TEST(Notifications().empty(), (rebuild));
    if (rebuild)
      Rebuild(200, 0);
    RunOnGui([&] { Depart(200, 0 /* fromY */, 40 /* firstOffset */); });
    TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), (rebuild));
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, LateRebuildPreservesObservedRejoin)
{
  for (bool const following : {false, true})
  {
    Build();
    if (following)
      Start();
    RunOnGui([&]
    {
      Feed(200, 0);
      Depart(200);

      // Calculate from the departure position before a newer fix rejoins the retained route.
      auto data = std::make_shared<TrackFollowingData>();
      data->m_points = m_track;
      TrackRouter router(VehicleType::Pedestrian, data);
      RoutesResult result;
      RouterDelegate delegate;
      TEST_EQUAL(router.CalculateRoute(Checkpoints(Point(200, 240), Point(1000, 0)), m2::PointD::Zero(), true, false,
                                       delegate, result),
                 RouterResultCode::NoError, ());
      Route incoming(result.GetActive());
      TEST(incoming.GetTrackStartDistanceMeters().has_value(), ());
      TEST_GREATER(*incoming.GetTrackStartDistanceMeters(), 0.0, ());

      m_session->SetState(SessionState::RouteRebuilding);
      Feed(400, 0);
      TEST(m_session->IsOnRoute(), ());
      // The old approach origin must not erase the rejoin observed while it was calculating.
      m_session->AssignRouteForTesting(std::move(incoming), RouterResultCode::NoError);
    });
    if (!following)
      Start();
    TEST(Notifications().empty(), (following));
    RunOnGui([&] { Depart(400, 0 /* fromY */, 40 /* firstOffset */); });
    TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), (following));
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, ManualBuildReplacesPendingTrackNotification)
{
  for (bool const approach : {false, true})
  {
    PrepareNotification(approach);
    Rebuild(200, 240, true);
    TEST(Notifications().empty(), ());
    Start();
    TEST_EQUAL(Notifications(), std::vector<std::string>({kApproach}), ());
    TEST(Notifications().empty(), ());
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, PreviewDepartureRequiresApproachOnStart)
{
  Build();
  RunOnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  TEST(Notifications().empty(), ());
  Rebuild(200, 240);
  Start();
  TEST_EQUAL(Notifications(), std::vector<std::string>({kApproach}), ());
  TEST(Notifications().empty(), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, StoppingClearsTrackNotifications)
{
  for (bool const approach : {false, true})
  {
    for (bool const reset : {false, true})
    {
      PrepareNotification(approach);
      if (!approach && !reset)
        Rebuild(200, 240);
      RunOnGui([&]
      {
        if (reset)
          m_session->Reset();
        else
          TEST(m_session->DisableFollowMode(), ());
      });
      TEST(Notifications().empty(), ());
    }
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, OrdinaryRouteKeepsRecalculationSpeech)
{
  Build(0, 0, true, false);
  Start();
  RunOnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  Rebuild(200, 240);
  TEST_EQUAL(Notifications(), std::vector<std::string>({kRecalculating}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, GeometricTurnsUseLocalizedSpeech)
{
  // Keep arrival beyond the announcement range so only bend instructions satisfy this test.
  m_track = {{Point(0, 0), 100}, {Point(200, 0), 100}, {Point(200, 1000), 100}};
  std::vector<std::string> previous;
  for (std::string const locale : {"en", "ru"})
  {
    Build();
    Start();
    std::vector<std::string> spoken;
    RunOnGui([&]
    {
      m_session->SetTurnNotificationsLocale(locale);
      for (double x : {0, 100, 150, 180, 195})
      {
        Feed(x, 0);
        std::vector<std::string> notifications;
        m_session->GenerateNotifications(notifications, true);
        spoken.insert(spoken.end(), notifications.begin(), notifications.end());
      }
    });
    TEST(!spoken.empty(), (locale));
    TEST_NOT_EQUAL(spoken, previous, (locale));
    previous = std::move(spoken);
  }
}
}  // namespace track_voice_tests
