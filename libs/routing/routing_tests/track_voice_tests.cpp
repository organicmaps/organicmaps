#include "testing/testing.hpp"

#include "routing/routing_tests/tools.hpp"

#include "routing/ruler_router.hpp"
#include "routing/track_router.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace track_voice_tests
{
using namespace routing;

std::string const kPhrase = "You have left the track.";
std::string const kRecalculating = "Recalculating route.";
std::string const kJson = R"({"you_have_left_the_track":"You have left the track.",
                             "route_recalculating":"Recalculating route."})";

m2::PointD Point(double x, double y)
{
  return {mercator::MetersToMercator(x), mercator::MetersToMercator(y)};
}

class Signal
{
public:
  void Set()
  {
    std::lock_guard lock(m_mutex);
    m_set = true;
    m_condition.notify_one();
  }

  void Wait()
  {
    std::unique_lock lock(m_mutex);
    TEST(m_condition.wait_for(lock, std::chrono::seconds(30), [this] { return m_set; }), ());
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_condition;
  bool m_set = false;
};

template <typename Fn>
void OnGui(Fn && fn)
{
  Signal done;
  GetPlatform().RunTask(Platform::Thread::Gui, [&]
  {
    fn();
    done.Set();
  });
  done.Wait();
}

class TrackVoiceTest : public AsyncGuiThreadTestWithRoutingSession
{
public:
  std::vector<geometry::PointWithAltitude> m_track = {{Point(0, 0), 100}, {Point(1000, 0), 100}};

  void Build(double x = 0, double y = 0, bool voice = true, bool track = true)
  {
    Signal built;
    OnGui([&]
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
      m_session->SetRoutingCallbacks([&](RoutesResult const &, RouterResultCode code)
      {
        TEST_EQUAL(code, RouterResultCode::NoError, ());
        built.Set();
      }, nullptr, nullptr, nullptr);
      m_session->BuildRoute(Checkpoints(Point(x, y), m_track.back().GetPoint()), RouterDelegate::kNoTimeout);
    });
    built.Wait();
    OnGui([&] { m_session->SetRoutingCallbacks(nullptr, nullptr, nullptr, nullptr); });
  }

  void Rebuild(double x, double y, bool manual = false)
  {
    Signal built;
    OnGui([&]
    {
      auto ready = [&](RoutesResult const &, RouterResultCode code)
      {
        TEST_EQUAL(code, RouterResultCode::NoError, ());
        built.Set();
      };
      if (manual)
      {
        m_session->SetRoutingCallbacks(ready, nullptr, nullptr, nullptr);
        m_session->BuildRoute(Checkpoints(Point(x, y), m_track.back().GetPoint()), RouterDelegate::kNoTimeout);
      }
      else
      {
        m_session->RebuildRoute(Point(x, y), ready, nullptr, nullptr, RouterDelegate::kNoTimeout,
                                SessionState::RouteRebuilding, true);
      }
    });
    built.Wait();
    OnGui([&] { m_session->SetRoutingCallbacks(nullptr, nullptr, nullptr, nullptr); });
  }

  void Start()
  {
    OnGui([&] { TEST(m_session->EnableFollowMode(), ()); });
  }

  void Voice(bool enabled)
  {
    OnGui([&] { m_session->EnableTurnNotifications(enabled); });
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
      auto bearing = std::atan2(x - m_x, y - m_y) * 180.0 / std::acos(-1.0);
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

  std::vector<std::string> Notifications()
  {
    std::vector<std::string> result;
    OnGui([&] { m_session->GenerateNotifications(result, false); });
    return result;
  }

private:
  double m_time = 100.0;
  double m_x = 0.0;
  double m_y = 0.0;
  bool m_hasFix = false;
};

UNIT_CLASS_TEST(TrackVoiceTest, DepartureAtTrackStart)
{
  Build();
  Start();
  OnGui([&]
  {
    Feed(0, 0);
    TEST(m_session->IsOnRoute(), ());
    Depart(0);
  });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
  TEST(Notifications().empty(), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, ExactApproachBoundaryCountsAsReached)
{
  Build(0, -100);
  Start();
  OnGui([&]
  {
    Feed(0, 0);
    TEST(m_session->IsOnRoute(), ());
    Depart(0);
  });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, ApproachMustReachTrack)
{
  Build(200, -100);
  Start();
  OnGui([&]
  {
    Feed(200, -100);
    for (double x = 220; x <= 440; x += 20)
      Feed(x, -100);
    TEST(!m_session->IsOnRoute(), ());
  });
  TEST(Notifications().empty(), ());
  Rebuild(440, -100);
  OnGui([&]
  {
    Feed(440, -100);
    for (double x = 460; x <= 680; x += 20)
      Feed(x, -100);
    TEST(!m_session->IsOnRoute(), ());
  });
  TEST(Notifications().empty(), ());
  OnGui([&]
  {
    // Reach this build's approach endpoint before advancing along the source track.
    Feed(440, 0);
    TEST(m_session->IsOnRoute(), ());
    Feed(700, 0);
    TEST(m_session->IsOnRoute(), ());
    Depart(700);
  });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, MutedDepartureIsSpokenOnce)
{
  Build(0, 0, false);
  Start();
  OnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  TEST(Notifications().empty(), ());
  Voice(true);
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
  TEST(Notifications().empty(), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, RejoinDropsMutedDeparture)
{
  Build(0, 0, false);
  Start();
  OnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  TEST(Notifications().empty(), ());
  OnGui([&]
  {
    Feed(400, 0);
    TEST(m_session->IsOnRoute(), ());
  });
  Voice(true);
  TEST(Notifications().empty(), ());
  OnGui([&] { Depart(400); });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, RebuildDoesNotRearmDeparture)
{
  Build(0, 0, false);
  Start();
  OnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  Rebuild(200, 240);
  Voice(true);
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
  OnGui([&]
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
  OnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
  OnGui([&]
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
  OnGui([&]
  {
    Feed(200, 0);
    TEST(m_session->IsOnRoute(), ());
  });
  Rebuild(200, 0);
  OnGui([&] { Depart(200, 0 /* fromY */, 40 /* firstOffset */); });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, LateRebuildPreservesObservedRejoin)
{
  Build();
  Start();
  OnGui([&]
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
    Depart(400, 0 /* fromY */, 40 /* firstOffset */);
  });
  TEST_EQUAL(Notifications(), std::vector<std::string>({kPhrase}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, ManualBuildClearsPendingDeparture)
{
  Build(0, 0, false);
  Start();
  OnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  Rebuild(200, 240, true);
  Voice(true);
  TEST(Notifications().empty(), ());
  Start();
  TEST(Notifications().empty(), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, PreviewDoesNotAnnounceDeparture)
{
  Build();
  OnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  TEST(Notifications().empty(), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, OrdinaryRouteKeepsRecalculationSpeech)
{
  Build(0, 0, true, false);
  Start();
  OnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  Rebuild(200, 240);
  TEST_EQUAL(Notifications(), std::vector<std::string>({kRecalculating}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, RouterChangeClearsPendingDeparture)
{
  Build(0, 0, false);
  Start();
  OnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  Build(0, 0, true, false);
  TEST(Notifications().empty(), ());
  Start();
  OnGui([&]
  {
    Feed(200, 0);
    Depart(200);
  });
  Rebuild(200, 240);
  TEST_EQUAL(Notifications(), std::vector<std::string>({kRecalculating}), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, MissingPhraseDoesNotEmitEmptyNotification)
{
  Build();
  Start();
  OnGui([&]
  {
    m_session->SetLocaleWithJsonForTesting(R"({"route_recalculating":"Recalculating route."})", "en");
    Feed(200, 0);
    Depart(200);
  });
  TEST(Notifications().empty(), ());
}

UNIT_CLASS_TEST(TrackVoiceTest, GeometricTurnsUseLocalizedSpeech)
{
  m_track = {{Point(0, 0), 100}, {Point(200, 0), 100}, {Point(200, 200), 100}};
  for (auto const & [locale, phrase] :
       std::vector<std::pair<std::string, std::string>>{{"en", "Turn left."}, {"ru", "Поверните налево."}})
  {
    Build();
    Start();
    std::vector<std::string> spoken;
    OnGui([&]
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
    TEST(std::find(spoken.begin(), spoken.end(), phrase) != spoken.end(), (locale, spoken));
  }
}
}  // namespace track_voice_tests
