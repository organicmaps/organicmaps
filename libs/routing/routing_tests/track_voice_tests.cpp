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
  void Feed(double x, double y, double accuracyM = 5.0, bool withBearing = true)
  {
    auto const distance = m_hasFix ? std::hypot(x - m_x, y - m_y) : 0.0;
    m_time += std::max(1.0, distance / 2.0);
    location::GpsInfo info;
    auto const latLon = mercator::ToLatLon(Point(x, y));
    info.m_latitude = latLon.m_lat;
    info.m_longitude = latLon.m_lon;
    info.m_timestamp = m_time;
    info.m_horizontalAccuracy = accuracyM;
    info.m_speed = 2.0;
    if (distance > 0.0 && withBearing)
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
      {
        RunOnGui([&] { Feed(0, 0); });
        // The manager refreshes the approach from the latest position before starting navigation.
        Rebuild(0, 0, true /* manual */);
      }
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

UNIT_CLASS_TEST(TrackVoiceTest, StartingPastEntryStaysOnTrack)
{
  for (double const start : {15.0, 19.0})
  {
    Build(start, 0);
    Start();
    RunOnGui([&]
    {
      for (double x = start; x < 200; x += 2)
      {
        Feed(x, 0);
        TEST(m_session->IsOnRoute(), (start, x));
      }
    });
    TEST(Notifications().empty(), ("Walking forward on the track must not announce departure", start));
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, TurningWithinEntryAccuracyJoinsTrack)
{
  for (double const shortBy : {2.0, 4.0})
  {
    Build(100, 0);
    Start();
    RunOnGui([&]
    {
      for (double x = 100; x > shortBy; x -= 2)
        Feed(x, 0);
      Feed(shortBy, 0);
      for (double x = shortBy + 2; x < 200; x += 2)
      {
        Feed(x, 0);
        TEST(m_session->IsOnRoute(), (shortBy, x));
      }
    });
    TEST(Notifications().empty(), ("Turning within GPS accuracy must reach the track", shortBy));
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
  RunOnGui([&]
  {
    Feed(202, 0);
    TEST(m_session->IsOnRoute(), ());
    TEST_ALMOST_EQUAL_ABS(mercator::DistanceOnEarth(m_session->GetRoute()->GetCurrentIter().m_pt, Point(202, 0)), 0.0,
                          1.0E-6, ("Continue on the source, not the rebuilt approach"));
    Depart(202, 0 /* fromY */, 40 /* firstOffset */);
  });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, PreviewGpsCannotSkipTheCapturedEntry)
{
  Build(0, -100);
  RunOnGui([&]
  {
    Feed(200, 0);
    TEST(!m_session->IsFollowing(), ());
    TEST_EQUAL(m_session->GetRoute()->GetCurrentDistanceFromBeginMeters(), 0.0, ());
  });
  TEST(Notifications().empty(), ());
  Rebuild(200, 0, true /* manual */);
  TEST(Notifications().empty(), ());
  Start();
  TEST_EQUAL(Notifications(), std::vector<std::string>({kApproach}), ());
  TEST(Notifications().empty(), ());
  RunOnGui([&]
  {
    // GPS movement during preview did not reach the captured entry at the track's beginning.
    Feed(0, 0);
    Feed(200, 0);
    TEST(m_session->IsOnRoute(), ());
  });
  RunOnGui([&] { Depart(200, 0 /* fromY */, 40 /* firstOffset */); });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, LateRebuildPreservesRejoinOnlyDuringNavigation)
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
      TEST_EQUAL(m_session->IsOnRoute(), following, ());
      // Only navigation observes a rejoin while the old approach is being calculated.
      m_session->AssignRouteForTesting(std::move(incoming), RouterResultCode::NoError);
    });
    if (!following)
    {
      TEST(Notifications().empty(), ());
      // Refresh the preview's approach without changing its captured entry before Start.
      Rebuild(400, 0, true /* manual */);
      Start();
      TEST_EQUAL(Notifications(), std::vector<std::string>({kApproach}), ());
      RunOnGui([&]
      {
        Feed(0, 0);
        Feed(400, 0);
        TEST(m_session->IsOnRoute(), ());
      });
    }
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
    TEST(!m_session->IsFollowing(), ());
    TEST_EQUAL(m_session->GetRoute()->GetCurrentDistanceFromBeginMeters(), 0.0, ());
  });
  TEST(Notifications().empty(), ());
  Rebuild(200, 240, true /* manual */);
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

UNIT_CLASS_TEST(TrackVoiceTest, JoinsDownstreamWithoutReachingSelectedEntry)
{
  Build(0, -300, false);
  Start();
  RunOnGui([&]
  {
    Feed(0, -300);
    for (int sample = 1; sample < 12; ++sample)
      Feed(200, -300 + sample * 25);
    TEST(!m_session->IsOnRoute(), ());
    Feed(200, 0);
    TEST(m_session->IsOnRoute(), ("Joining the source downstream must end the departure state"));
    TEST_ALMOST_EQUAL_ABS(m_session->GetRoute()->GetCurrentDistanceFromBeginMeters(), 500.0, 2.0, ());
    Feed(400, 0);
    Feed(1000, 0);
    TEST(m_session->IsFinished(), ("Arrival must remain available after a downstream join"));
  });
}

UNIT_CLASS_TEST(TrackVoiceTest, CoarseLoopFixesCannotInventACompletedLap)
{
  m_track = {{Point(0, 0), 100}, {Point(20, 0), 100}, {Point(20, 20), 100}, {Point(0, 20), 100}, {Point(0, 0), 100}};
  for (double const accuracy : {30.0, 1000.0})
  {
    Build();
    Start();
    RunOnGui([&]
    {
      Feed(0, 0);
      for (auto const & fix : {m2::PointD(20, 0), m2::PointD(20, 20), m2::PointD(0, 20), m2::PointD(0, 0)})
      {
        // At 30 m accuracy this whole loop can be jitter around a stationary receiver.
        Feed(fix.x, fix.y, accuracy);
        TEST(m_session->IsOnRoute(), (accuracy));
        TEST_EQUAL(m_session->GetRoute()->GetCurrentDistanceFromBeginMeters(), 0.0, (accuracy));
      }
      Feed(0, 0);
      TEST(!m_session->IsFinished(), ("A subsequent accurate fix cannot validate a noisy lap", accuracy));
    });
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, SkippedSpurRejoinsAndArrives)
{
  m_track = {
      {Point(0, 0), 100}, {Point(500, 0), 100}, {Point(500, 300), 100}, {Point(500, 0), 101}, {Point(1000, 0), 100}};
  for (bool const withBearing : {false, true})
  {
    Build();
    Start();
    RunOnGui([&]
    {
      for (double x = 0; x <= 1000; x += 2)
      {
        Feed(x, 0, 5.0, withBearing);
        if (x >= 550)
          TEST(m_session->IsOnRoute() || m_session->IsFinished(), (x, withBearing));
        std::vector<std::string> notifications;
        m_session->GenerateNotifications(notifications, false);
        TEST(std::find(notifications.begin(), notifications.end(), kPhrase) == notifications.end(),
             ("Skipping the spur must not announce a false departure", x, withBearing));
      }
      TEST(m_session->IsFinished(), (withBearing));
    });
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, CutSwitchbacksRejoinAndArrive)
{
  m_track = {{Point(0, 0), 100},   {Point(1000, 0), 100}, {Point(1000, 100), 100},
             {Point(0, 100), 100}, {Point(0, 200), 100},  {Point(1000, 200), 100}};
  for (bool const withBearing : {false, true})
  {
    Build();
    Start();
    RunOnGui([&]
    {
      for (double x = 0; x <= 500; x += 2)
        Feed(x, 0, 5.0, withBearing);
      for (double y = 2; y <= 200; y += 2)
        Feed(500, y, 5.0, withBearing);
      for (double x = 502; x <= 1000; x += 2)
      {
        Feed(x, 200, 5.0, withBearing);
        if (x >= 550)
          TEST(m_session->IsOnRoute() || m_session->IsFinished(), (x, withBearing));
      }
      TEST(m_session->IsFinished(), (withBearing));
    });
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, DownstreamRejoinRejectsUnconfirmedMovement)
{
  m_track = {{Point(0, 0), 100},   {Point(1000, 0), 100}, {Point(1000, 100), 100},
             {Point(0, 100), 100}, {Point(0, 200), 100},  {Point(1000, 200), 100}};
  Build();
  Start();
  RunOnGui([&]
  {
    Feed(0, 0);
    Feed(500, 0);
    double const progress = m_session->GetRoute()->GetCurrentDistanceFromBeginMeters();
    for (int i = 0; i < 10; ++i)
    {
      // Repeated fixes and jitter on a downstream leg cannot prove traversal.
      for (double x : {500, 500, 502, 504, 502})
      {
        Feed(x, 200);
        TEST_EQUAL(m_session->GetRoute()->GetCurrentDistanceFromBeginMeters(), progress, (i, x));
      }
    }
    for (double x = 520; x <= 560; x += 2)
    {
      Feed(x, 200, 100.0);
      TEST_EQUAL(m_session->GetRoute()->GetCurrentDistanceFromBeginMeters(), progress, (x));
    }
    // A later eastbound occurrence must not be claimed while walking west.
    for (double x = 500; x >= 450; x -= 2)
    {
      Feed(x, 200);
      TEST_EQUAL(m_session->GetRoute()->GetCurrentDistanceFromBeginMeters(), progress, (x));
    }
    for (double x = 452; x <= 550; x += 2)
      Feed(x, 200);
    TEST(m_session->IsOnRoute(), ());
    TEST_EQUAL(m_session->GetRoute()->GetCurrentIter().m_ind, 5, ());
  });
}

UNIT_CLASS_TEST(TrackVoiceTest, RejoinKeepsEarliestDownstreamOccurrence)
{
  m_track = {{Point(0, 0), 100},     {Point(500, 0), 100},  {Point(500, 300), 100},
             {Point(500, 0), 101},   {Point(1000, 0), 100}, {Point(1000, 400), 100},
             {Point(500, 400), 100}, {Point(500, 0), 100},  {Point(1000, 0), 100}};
  Build();
  Start();
  RunOnGui([&]
  {
    for (double x = 0; x <= 600; x += 2)
      Feed(x, 0);
    TEST(m_session->IsOnRoute(), ());
    TEST_EQUAL(m_session->GetRoute()->GetCurrentIter().m_ind, 4, ());
    TEST_ALMOST_EQUAL_ABS(m_session->GetRoute()->GetCurrentDistanceFromBeginMeters(), 1200.0, 3.0, ());
  });
}

UNIT_CLASS_TEST(TrackVoiceTest, RejoinDoesNotAlternateBetweenNearbyVisits)
{
  m_track = {{Point(0, 0), 100},   {Point(1000, 0), 100},   {Point(1000, 100), 100}, {Point(0, 100), 100},
             {Point(0, 200), 100}, {Point(1000, 200), 100}, {Point(1000, 500), 100}, {Point(0, 500), 100},
             {Point(0, 202), 100}, {Point(1000, 202), 100}};
  Build();
  Start();
  RunOnGui([&]
  {
    Feed(0, 0);
    Feed(500, 0);
    double const progress = m_session->GetRoute()->GetCurrentDistanceFromBeginMeters();
    // Adjacent eastbound legs are separated by a long source detour. Alternating
    // between them must not accumulate confirmations of either occurrence.
    for (int sample = 0; sample < 10; ++sample)
    {
      Feed(500 + 8 * sample, sample % 2 == 0 ? 200 : 202, 1.0);
      TEST_EQUAL(m_session->GetRoute()->GetCurrentDistanceFromBeginMeters(), progress, (sample));
    }
    for (double x = 580; x <= 620; x += 8)
      Feed(x, 200, 1.0);
    TEST(m_session->IsOnRoute(), ());
    TEST_EQUAL(m_session->GetRoute()->GetCurrentIter().m_ind, 5, ());
  });
}

UNIT_CLASS_TEST(TrackVoiceTest, DenseNoisyRecordingKeepsFollowingAndArrives)
{
  for (double const noiseM : {1.0, 2.0})
  {
    m_track.clear();
    for (int x = 0; x <= 1000; ++x)
      m_track.emplace_back(Point(x, x % 2 == 0 ? noiseM : -noiseM), 100);
    Build();
    Start();
    RunOnGui([&]
    {
      for (int sample = 0; sample <= 725; ++sample)
      {
        Feed(std::min(1000.0, sample * 1.4), 0.0);
        TEST(m_session->IsOnRoute() || m_session->IsFinished(), (noiseM, sample));
        std::vector<std::string> notifications;
        m_session->GenerateNotifications(notifications, false);
        TEST(std::find(notifications.begin(), notifications.end(), kPhrase) == notifications.end(), (noiseM, sample));
      }
      TEST(m_session->IsFinished(), ("Accurate fixes at the finish must detect arrival", noiseM));
    });
  }
}

UNIT_CLASS_TEST(TrackVoiceTest, DepartureKeepsCompletionAndSuppressesStaleTurns)
{
  m_track = {{Point(0, 0), 100}, {Point(200, 0), 100}, {Point(200, 300), 100}};
  Build(0, 0, false);
  Start();
  double completion = 0.0;
  FollowingInfo info;
  RunOnGui([&]
  {
    m_session->SetTurnNotificationsLocale("en");
    Feed(0, 0);
    Feed(190, 0);
    m_session->GetRouteFollowingInfo(info);
    TEST_EQUAL(info.m_pedestrianTurn, turns::PedestrianDirection::TurnLeft, ());
    TEST(info.m_distToTurn.IsValid(), ());
    completion = m_session->GetCompletionPercent();
    for (double y = -40; y >= -400; y -= 40)
      Feed(190, y);
    TEST(!m_session->IsOnRoute(), ());
    TEST(m_session->IsNavigable(), ("Keep remaining metrics and navigation presentation available off track"));
    m_session->GetRouteFollowingInfo(info);
    TEST(info.m_distToTarget.IsValid(), ());
    TEST_GREATER(info.m_time, 0, ());
    TEST_ALMOST_EQUAL_ABS(info.m_completionPercent, completion, 1.0E-6, ());
    TEST_EQUAL(info.m_pedestrianTurn, turns::PedestrianDirection::None, ());
    TEST_EQUAL(info.m_turn, turns::CarDirection::None, ());
    TEST_EQUAL(info.m_nextTurn, turns::CarDirection::None, ());
    TEST(!info.m_distToTurn.IsValid(), ());
    TEST(info.m_currentStreetName.empty() && info.m_nextStreetName.empty(), ());
    TEST(info.m_lanes.empty(), ());
  });
  Voice(true);
  auto const containsTurn = [](std::vector<std::string> const & notifications)
  {
    return std::any_of(notifications.begin(), notifications.end(),
                       [](auto const & text) { return text.find("Turn left.") != std::string::npos; });
  };
  for (size_t i = 0; i < 3; ++i)
    TEST(!containsTurn(Notifications()), ("An off-track iterator must not announce stale turns"));
  RunOnGui([&]
  {
    Feed(190, 0);
    TEST(m_session->IsOnRoute(), ());
    m_session->GetRouteFollowingInfo(info);
    TEST_ALMOST_EQUAL_ABS(info.m_completionPercent, completion, 1.0E-6, ());
    TEST_EQUAL(info.m_pedestrianTurn, turns::PedestrianDirection::TurnLeft, ());
    TEST(info.m_distToTurn.IsValid(), ());
  });
  bool resumedTurns = false;
  for (size_t i = 0; i < 3; ++i)
    resumedTurns = containsTurn(Notifications()) || resumedTurns;
  TEST(resumedTurns, ("Normal turn speech resumes after rejoining"));
}

UNIT_CLASS_TEST(TrackVoiceTest, OffTrackRebuildDoesNotRestoreStaleTurns)
{
  m_track = {{Point(0, 0), 100}, {Point(200, 0), 100}, {Point(200, 300), 100}};
  Build(0, 0, false);
  Start();
  RunOnGui([&]
  {
    m_session->SetTurnNotificationsLocale("en");
    Feed(0, 0);
    Feed(190, 0);
    for (double y = -40; y >= -400; y -= 40)
      Feed(190, y);
    TEST(!m_session->IsOnRoute(), ());
  });
  Rebuild(190, -400);
  Voice(true);
  RunOnGui([&]
  {
    FollowingInfo info;
    m_session->GetRouteFollowingInfo(info);
    TEST_EQUAL(info.m_pedestrianTurn, turns::PedestrianDirection::None, ());
    TEST_EQUAL(m_session->GetRoute()->GetCurrentIter().m_ind, 0, ("Start at the new approach"));
  });
  for (int i = 0; i < 3; ++i)
    TEST(Notifications().empty(), ("A rebuild 400 m off track must not speak the old turn"));
}

UNIT_CLASS_TEST(TrackVoiceTest, WindingTrackRejoinBeforeLateRebuildKeepsProgress)
{
  m_track = {
      {Point(0, 0), 100}, {Point(400, 0), 100}, {Point(400, 400), 100}, {Point(0, 400), 100}, {Point(0, 1000), 100}};
  Build();
  Start();
  RunOnGui([&]
  {
    for (auto const & point : {m2::PointD(0, 0), m2::PointD(400, 0), m2::PointD(400, 400), m2::PointD(0, 400)})
      Feed(point.x, point.y);
    for (double x = -40; x >= -240; x -= 40)
      Feed(x, 400);
    TEST(!m_session->IsOnRoute(), ());

    auto data = std::make_shared<TrackFollowingData>();
    data->m_points = m_track;
    TrackRouter router(VehicleType::Pedestrian, data);
    RoutesResult result;
    RouterDelegate delegate;
    TEST_EQUAL(router.CalculateRoute(Checkpoints(Point(-240, 400), Point(0, 1000)), m2::PointD::Zero(), true, false,
                                     delegate, result),
               RouterResultCode::NoError, ());
    m_session->SetState(SessionState::RouteRebuilding);
    Feed(0, 402);
    TEST(m_session->IsOnRoute(), ("Rejoin on the retained route before the rebuild completes"));
    m_session->AssignRouteForTesting(Route(result.GetActive()), RouterResultCode::NoError);
    for (double y = 404; y <= 600; y += 2)
    {
      Feed(0, y);
      TEST(m_session->IsOnRoute(), (y));
      TEST_ALMOST_EQUAL_ABS(mercator::DistanceOnEarth(m_session->GetRoute()->GetCurrentIter().m_pt, Point(0, y)), 0.0,
                            1.0E-6, (y));
    }
  });
}

}  // namespace track_voice_tests
