#include "app/organicmaps/sdk/Framework.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/extrapolation/extrapolator.hpp"
#include "map/nogps/clock.hpp"
#include "map/nogps/delegate.hpp"
#include "map/nogps/engine.hpp"
#include "map/nogps/esp32_protocol.hpp"
#include "map/nogps/geo.hpp"
#include "map/nogps/map_api.hpp"
#include "map/nogps/scheduler.hpp"
#include "map/nogps/service.hpp"
#include "map/nogps/storage.hpp"
#include "map/routing_manager.hpp"

#include "routing/checkpoints.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_session.hpp"

#include "platform/location.hpp"
#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// A recorded trip driven again, for debug builds: everything the navigation without GPS was given on a trip, taken
// from its trip log, goes through a navigation of its own on the roads of the map, as fast as the phone counts, and
// what it logs is written to a file as a trip log. It tells what a new rule does on the trips already driven, where
// GPS was seen, before the user drives with it. The route the user has asked for on the trip is built again by a
// router of its own and lives as the route of the application does, see ReplayRoute.
//
// The input is made by tools/python/nogps_replay.py, a record a line, the time is milliseconds of the day:
//   D <MM-DD>                     the date for the stamps of the output
//   S <ms> <0|1|2>                the navigation is stopped, started, or the application is started anew
//   J <ms>                        the time jumps on: nothing has happened meanwhile
//   L <ms> <state>                Bluetooth of the box, as DebugPrint(BleState) tells it
//   B <ms> <duration ms> <yaw mdeg> <km/h> <flags hex> <h1> <h2> <up>
//                                 a part of a second of the box that ends at the time: its rotation, the speed of
//                                 the car, the acceleration in mm/s2 or "-" without it
//   G <ms> <lat> <lon> <accuracy m> <m/s or -> <bearing or -> <its accuracy or ->     a position of GPS
//   X <ms> <0|1>   A <ms> <0|1>   the user turns GPS off or on, the manual mode on or off
//   T <ms> <lat> <lon>            the user taps the map
//   H <ms> <bearing>              the mark looked this way after that tap: it is turned if it doesn't
//   R <ms>   P <ms> <0|1>   M <ms> <meters>
//                                 the reverse button, the pause button of the trips before October 16 (there is no
//                                 pause since then), the mark moved along the road
//   W <ms>                        the button showing the car on another road it may be on
//   N <ms> <lat> <lon> <lat> <lon> ...
//                                 the user asks for a route through the points, the first one is where the car is
//   F <ms> <0|1>                  the user closes the route, or starts to follow it
namespace
{
using namespace nogps;

class ReplayClock
  : public Clock
  , public Scheduler
{
public:
  int64_t NowMs() const override { return m_nowMs; }
  int64_t UnixNowMs() const override { return m_nowMs + 1'700'000'000'000; }

  // Far from zero, as the time since the boot of a phone.
  static int64_t constexpr kStartMs = 86'400'000LL * 1000;

  void Post(int64_t delayMs, Task && task) override
  {
    m_tasks.push_back({m_nowMs + delayMs, m_nextId++, std::move(task)});
  }

  // Moves the time on, running the tasks that come due on the way in their order.
  void AdvanceTo(int64_t target)
  {
    while (true)
    {
      auto const next = std::min_element(m_tasks.begin(), m_tasks.end(), [](Pending const & a, Pending const & b)
      { return a.m_dueMs != b.m_dueMs ? a.m_dueMs < b.m_dueMs : a.m_id < b.m_id; });
      if (next == m_tasks.end() || next->m_dueMs > target)
        break;
      m_nowMs = std::max(m_nowMs, next->m_dueMs);
      auto task = std::move(next->m_task);
      m_tasks.erase(next);
      task();
    }
    m_nowMs = std::max(m_nowMs, target);
  }

  // The time between two trips: nothing happens in it, the tasks waiting run once after it.
  void JumpTo(int64_t target) { m_nowMs = std::max(m_nowMs, target); }

private:
  struct Pending
  {
    int64_t m_dueMs;
    uint64_t m_id;
    Task m_task;
  };

  int64_t m_nowMs = kStartMs;
  uint64_t m_nextId = 0;
  std::vector<Pending> m_tasks;
};

// What the navigation has learned about the car is taken from the settings of the application, nothing is written
// back to them. A trip driven again as the first one knows no speed errors yet: the lag of the speed and the
// accelerometer, learned apart from them, are still taken.
class ReplayStorage : public Storage
{
public:
  explicit ReplayStorage(bool firstTrip)
  {
    SettingsStorage const settings;
    for (auto const key : {kSpeedScale, kSpeedTable, kRoughSpeedTable, kSpeedLag, kBoxAccel})
    {
      if (firstTrip && (key == kSpeedScale || key == kSpeedTable || key == kRoughSpeedTable))
        continue;
      if (auto const value = settings.GetString(key))
        m_values[std::string(key)] = *value;
    }
    Set(kInertialEnabled, true);
    Set(kEsp32Source, true);
  }

  std::optional<std::string> GetString(std::string_view key) const override
  {
    auto const it = m_values.find(std::string(key));
    if (it == m_values.end())
      return {};
    return it->second;
  }

  void SetString(std::string_view key, std::string const & value) override { m_values[std::string(key)] = value; }

private:
  std::map<std::string, std::string> m_values;
};

class ReplayDelegate : public Delegate
{
public:
  using PositionFn = std::function<void(Fix const &)>;

  explicit ReplayDelegate(PositionFn && onPosition) : m_onPosition(std::move(onPosition)) {}

  void StartMotionSensors(bool) override {}
  void StopMotionSensors() override {}
  void Elm327Connect(std::string const &) override {}
  void Elm327Write(std::string const &) override {}
  void Elm327Close() override {}
  void Esp32BleOpen() override {}
  void Esp32BleSend(std::string const &) override {}
  void Esp32BleSendFirmware(std::string const &) override {}
  void Esp32BleClose() override {}
  void OnPosition(Fix const & fix) override { m_onPosition(fix); }
  void OnEvent(Event event) override { LOG(LINFO, ("EVENT", DebugPrint(event))); }

private:
  PositionFn m_onPosition;
};

// The route of the trip: a routing session of its own with a router as the one of the application, so the route
// of the user is not touched. It is built where the trip log tells the user has asked for it. From then on it
// lives as the route of the application does: it is given the positions the navigation shows, as Framework and
// RoutingManager give them, it is rebuilt when the car leaves it, and its alternative is followed when the car
// turns to one. So it is the route the application would have with the mark driven again, which is not the route
// of the trip where that mark goes another way.
class ReplayRoute
{
public:
  using LogFn = std::function<void(std::string const &)>;

  ReplayRoute(RoutingManager const & routing, ReplayClock & clock, LogFn && log)
    : m_clock(clock)
    , m_log(std::move(log))
    , m_route(m_session, std::bind_front(&ReplayRoute::OnBuilt, this, "rebuilt"),
              [this](size_t idx) { return m_session.SwapActiveAlternative(idx); })
  {
    m_session.Init(nullptr /* pointCheckCallback */);
    m_session.SetRoutingCallbacks(
        std::bind_front(&ReplayRoute::OnBuilt, this, "built"), std::bind_front(&ReplayRoute::OnBuilt, this, "rebuilt"),
        [this](uint64_t, std::set<std::string> const &) { m_log("ROUTE needs more maps"); },
        [this](routing::RouterResultCode code) { m_log("ROUTE is removed: " + DebugPrint(code)); });
    m_session.SetChangeSessionStateCallback(std::bind_front(&ReplayRoute::OnState, this));
    m_session.SetSpeedCamShowCallback([](m2::PointD const &, double) {});
    m_session.SetSpeedCamClearCallback([] {});
    routing.InitSession(m_session, routing::RouterType::Vehicle);
  }

  SessionRoute & Get() { return m_route; }

  // The router works in its own thread and its route comes later: the trip waits for it. The state of the session
  // does not tell it: a car that has left the route for a road beside it is on the old route for the session again
  // at its next position.
  bool IsBuilding() const { return m_isBuilding; }

  // The states of the session are logged by the session while the trip is driven, see Replay::RunChunk().
  void SetDriving(bool driving) { m_driving = driving; }

  // As RoutingManager::BuildRoute() does.
  void Build(std::vector<m2::PointD> && points)
  {
    if (points.size() < 2)
      return;
    if (m_session.IsActive())
      Close();
    m_session.ClearPositionAccumulator();
    m_session.SetUserCurrentPosition(points.front());
    m_session.BuildRoute(routing::Checkpoints(std::move(points)), routing::RouterDelegate::kNoTimeout);
  }

  // As RoutingManager::FollowRoute() and CloseRouting() do.
  void Follow() { m_session.EnableFollowMode(); }

  void Close()
  {
    m_session.Reset();
    m_isBuilding = false;
  }

  // A position the navigation shows. The application gives the route every position 10 times in 2 seconds, moved
  // on from the previous one, see extrapolation::Extrapolator: so many positions off the route rebuild it.
  void OnPosition(Fix const & fix)
  {
    location::GpsInfo info;
    info.m_source = location::EAndroidNative;
    info.m_timestamp = fix.m_unixTimeMs / 1000.0;
    info.m_latitude = fix.m_position.m_lat;
    info.m_longitude = fix.m_position.m_lon;
    if (fix.m_accuracyM > 0)
      info.m_horizontalAccuracy = fix.m_accuracyM;
    if (fix.m_bearingDeg)
      info.m_bearing = *fix.m_bearingDeg;
    if (fix.m_speedMps)
      info.m_speed = *fix.m_speedMps;
    m_beforeLast = m_last;
    m_last = info;
    m_runs = 0;
    // The application gives it to the route after the navigation is done with it.
    m_clock.Post(0, [this, generation = ++m_generation] { Extrapolate(generation); });
  }

private:
  void Extrapolate(uint64_t generation)
  {
    using extrapolation::Extrapolator;
    uint64_t const afterMs = Extrapolator::kExtrapolationPeriodMs * m_runs;
    if (generation != m_generation || afterMs >= Extrapolator::kMaxExtrapolationTimeMs)
      return;
    ++m_runs;
    bool const moved = extrapolation::AreCoordsGoodForExtrapolation(m_beforeLast, m_last);
    Give(moved ? extrapolation::LinearExtrapolation(m_beforeLast, m_last, afterMs) : m_last);
    m_clock.Post(Extrapolator::kExtrapolationPeriodMs, [this, generation] { Extrapolate(generation); });
  }

  // As RoutingManager::GetRouteMatchingInfo() and Framework::OnUserPositionChanged() do.
  void Give(location::GpsInfo info)
  {
    m_route.CheckLocationForRouting(info);
    if (m_session.IsActive())
    {
      location::RouteMatchingInfo matching;
      if (!m_session.MatchLocationToRoute(info, matching))
        m_session.MatchLocationToRoadGraph(info);
    }
    // The map tells back the position it shows: on the route, if the car is on it.
    m2::PointD const shown = mercator::FromLatLon(info.m_latitude, info.m_longitude);
    m_session.PushPositionAccumulator(shown);
    if (m_session.IsActive())
      m_session.SetUserCurrentPosition(shown);
  }

  void OnBuilt(char const * what, routing::RoutesResult const & result, routing::RouterResultCode code)
  {
    m_isBuilding = false;
    std::ostringstream out;
    out << "ROUTE is " << what << ": " << DebugPrint(code);
    // The session has taken the route by now.
    if (auto const * route = m_session.GetRoute(); code == routing::RouterResultCode::NoError && route != nullptr)
    {
      out << ", " << std::lround(route->GetTotalDistanceMeters()) << " m, " << route->GetPoly().GetPoints().size()
          << " points, routes = " << result.m_routes.size();
    }
    m_log(out.str());
  }

  void OnState(routing::SessionState from, routing::SessionState to)
  {
    if (to == routing::SessionState::RouteBuilding || to == routing::SessionState::RouteRebuilding)
      m_isBuilding = true;
    // A failed rebuild keeps the route without OnBuilt().
    else if (from == routing::SessionState::RouteRebuilding &&
             (to == routing::SessionState::RouteNotStarted || to == routing::SessionState::NoValidRoute))
      m_isBuilding = false;
    if (!m_driving)
      m_log("Routing session state: " + DebugPrint(from) + " -> " + DebugPrint(to));
  }

  ReplayClock & m_clock;
  LogFn m_log;
  routing::RoutingSession m_session;
  SessionRoute m_route;
  bool m_driving = false;
  bool m_isBuilding = false;
  location::GpsInfo m_beforeLast;
  location::GpsInfo m_last;
  uint64_t m_runs = 0;
  uint64_t m_generation = 0;
};

class Replay;
Replay * g_logTo = nullptr;
base::LogMessageFn g_previousLog = nullptr;
std::thread::id g_replayThread;

void ReplayLogMessage(base::LogLevel level, base::SrcPoint const & src, std::string const & message);

class Replay
{
public:
  Replay(RoutingManager & routing, std::string const & inputPath, std::string const & outputPath)
    : m_input(inputPath)
    , m_output(outputPath, std::ios::trunc)
    , m_donePath(outputPath + ".done")
    // nogps-replay-fresh.txt next to the input tells to drive the trip as the first one: the application renames
    // the input before it is driven.
    , m_storage(std::ifstream(inputPath.substr(0, inputPath.find_last_of('/') + 1) + "nogps-replay-fresh.txt").good())
    , m_route(routing, m_clock, [this](std::string const & text) { Write(text); })
    , m_delegate([this](Fix const & fix) { m_route.OnPosition(fix); })
    , m_map(m_route.Get(), m_clock, [](double) {}, [](std::optional<ms::LatLon> const &) {})
  {
    std::remove(m_donePath.c_str());
    NewEngine();
  }

  // Drives on for a part of a second of the phone. \returns false when the trip is over.
  bool RunChunk()
  {
    // The time of the trip stands while the router builds a route: on the trip it took a part of a second.
    if (m_route.IsBuilding())
    {
      auto const now = std::chrono::steady_clock::now();
      if (!m_waitsSince)
        m_waitsSince = now;
      if (now - *m_waitsSince < std::chrono::seconds(60))
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return true;
      }
      Write("ROUTE is not built in a minute, it is closed");
      m_route.Close();
    }
    m_waitsSince.reset();

    g_logTo = this;
    g_replayThread = std::this_thread::get_id();
    g_previousLog = base::SetLogMessageFn(&ReplayLogMessage);
    m_route.SetDriving(true);
    auto const started = std::chrono::steady_clock::now();
    bool more = true;
    std::string line;
    while (std::chrono::steady_clock::now() - started < std::chrono::milliseconds(120) && !m_route.IsBuilding())
    {
      if (!std::getline(m_input, line))
      {
        more = false;
        break;
      }
      OnRecord(line);
    }
    if (!more)
    {
      m_engine->Stop();
      m_output.flush();
    }
    m_route.SetDriving(false);
    base::SetLogMessageFn(g_previousLog);
    g_logTo = nullptr;
    if (!more)
    {
      std::ofstream(m_donePath) << m_records << " records\n";
      LOG(LINFO, ("The trip is driven again:", m_records, "records, written to", m_donePath));
    }
    return more;
  }

  void Write(std::string const & text)
  {
    int64_t const ms = m_clock.NowMs() % 86'400'000;
    char stamp[48];
    std::snprintf(stamp, sizeof(stamp), "%s %02d:%02d:%02d.%03d (replay) I/OMcore: ", m_date.c_str(),
                  static_cast<int>(ms / 3'600'000), static_cast<int>(ms / 60'000 % 60),
                  static_cast<int>(ms / 1000 % 60), static_cast<int>(ms % 1000));
    m_output << stamp << text << '\n';
  }

private:
  void OnRecord(std::string const & line)
  {
    std::istringstream in(line);
    char kind = 0;
    int64_t ms = 0;
    in >> kind;
    if (kind == 'D')
    {
      in >> m_date;
      return;
    }
    if (!(in >> ms))
      return;
    ++m_records;
    ms += ReplayClock::kStartMs;
    // The time of a part of the box is its end, the time goes on through it.
    int64_t durationMs = 0;
    if (kind == 'B' && !(in >> durationMs))
      return;
    if (kind == 'J' || !m_timeSet)
      m_clock.JumpTo(ms - durationMs);
    else
      m_clock.AdvanceTo(ms - durationMs);
    m_timeSet = true;
    Engine & engine = *m_engine;
    switch (kind)
    {
    case 'B': OnBox(in, ms, durationMs); break;
    case 'S':
    {
      int state = 0;
      in >> state;
      if (state == 2)
      {
        // The application started anew has no route until the user asks for one.
        m_route.Close();
        NewEngine();
      }
      else if (state == 1)
        engine.Start();
      else
        engine.Stop();
      break;
    }
    case 'L':
    {
      std::string name;
      in >> name;
      for (auto const state : {BleState::Off, BleState::NoPermission, BleState::Disabled, BleState::Searching,
                               BleState::PairingClosed, BleState::Connecting, BleState::Pairing, BleState::Connected})
      {
        if (DebugPrint(state) == name)
          engine.OnEsp32BleState(state);
      }
      break;
    }
    case 'G':
    {
      Fix fix;
      std::string speed;
      std::string bearing;
      if (!(in >> fix.m_position.m_lat >> fix.m_position.m_lon >> fix.m_accuracyM >> speed >> bearing))
        break;
      if (speed != "-")
        fix.m_speedMps = std::stod(speed);
      if (bearing != "-")
        fix.m_bearingDeg = std::stod(bearing);
      if (std::string accuracy; (in >> accuracy) && accuracy != "-")
        fix.m_bearingAccuracyDeg = std::stod(accuracy);
      fix.m_timeMs = m_clock.NowMs();
      fix.m_unixTimeMs = m_clock.UnixNowMs();
      engine.OnFix(fix);
      break;
    }
    case 'X':
    {
      int disabled = 0;
      if (in >> disabled)
        engine.SetGpsDisabled(disabled != 0);
      break;
    }
    case 'A':
    {
      int enabled = 0;
      if (in >> enabled)
        engine.SetManualMode(enabled != 0);
      break;
    }
    case 'T':
    {
      // The buttons and the taps of the manual mode do nothing without it, as in the application.
      double lat = 0;
      double lon = 0;
      if ((in >> lat >> lon) && engine.IsManualMode())
        engine.PlaceMarkByTap({lat, lon});
      break;
    }
    case 'H':
    {
      double bearing = 0;
      auto const heading = GetHeading();
      if ((in >> bearing) && engine.IsManualMode() && heading && std::fabs(AngleDiff(*heading, bearing)) > 90)
        engine.ReverseDirection();
      break;
    }
    case 'R':
      if (engine.IsManualMode())
        engine.ReverseDirection();
      break;
    case 'W':
      if (engine.IsManualMode())
        engine.SwitchMark();
      break;
    case 'M':
    {
      double meters = 0;
      if ((in >> meters) && engine.IsManualMode())
        engine.ShiftPosition(meters);
      break;
    }
    case 'N':
    {
      std::vector<m2::PointD> points;
      double lat = 0;
      double lon = 0;
      while (in >> lat >> lon)
        points.push_back(mercator::FromLatLon(lat, lon));
      LOG(LINFO, ("ROUTE is asked for through", points.size(), "points"));
      m_route.Build(std::move(points));
      break;
    }
    case 'F':
    {
      int follow = 0;
      if (!(in >> follow))
        break;
      if (follow != 0)
        m_route.Follow();
      else
        m_route.Close();
      break;
    }
    default: break;
    }
  }

  // Where the car looks, as the trip log tells it.
  std::optional<double> GetHeading() const
  {
    std::string const line = m_engine->GetTripLine();
    auto const pos = line.find(" hdg=");
    if (pos == std::string::npos || pos + 5 >= line.size() || line[pos + 5] < '0' || line[pos + 5] > '9')
      return {};
    return std::atof(line.c_str() + pos + 5);
  }

  // The application started anew has a new navigation: what it keeps is in the storage.
  void NewEngine()
  {
    m_engine.reset();
    m_engine = std::make_unique<Engine>(m_delegate, m_map, m_storage, m_clock, m_clock);
  }

  // The box sends its data 50 times a second, the log keeps 5 parts of a second: every part is sent as 10 lines.
  void OnBox(std::istringstream & in, int64_t ms, int64_t durationMs)
  {
    static int constexpr kLines = 10;
    int64_t yawMdeg = 0;
    int speedKmh = 0;
    std::string flags;
    std::string h1;
    if (!(in >> yawMdeg >> speedKmh >> flags >> h1))
      return;
    int h2 = 0;
    int up = 0;
    bool const hasAccel = h1 != "-" && (in >> h2 >> up);
    int64_t const startMdeg = m_yawMdeg;
    for (int i = 1; i <= kLines; ++i)
    {
      // The time of the box goes on by itself: a record driven in the middle of the part has moved the clock.
      int64_t const boxMs = ms - durationMs + durationMs * i / kLines;
      m_clock.AdvanceTo(boxMs);
      char body[200];
      int const length =
          std::snprintf(body, sizeof(body), "NGD,1,%lld,%lld,%lld,0,0,%d,0,%s", static_cast<long long>(++m_seq),
                        static_cast<long long>(boxMs), static_cast<long long>(startMdeg + yawMdeg * i / kLines),
                        speedKmh, flags.c_str());
      if (hasAccel)
        std::snprintf(body + length, sizeof(body) - length, ",%s,%d,%d,0", h1.c_str(), h2, up);
      char crc[8];
      std::snprintf(crc, sizeof(crc), "%04X", esp32::Crc16(body));
      m_engine->OnEsp32BleBytes(std::string("$") + body + "*" + crc + "\r\n");
    }
    m_yawMdeg = startMdeg + yawMdeg;
  }

  std::ifstream m_input;
  std::ofstream m_output;
  std::string const m_donePath;
  std::string m_date = "01-01";
  ReplayClock m_clock;
  ReplayStorage m_storage;
  ReplayRoute m_route;
  ReplayDelegate m_delegate;
  RoutingMapApi m_map;
  std::unique_ptr<Engine> m_engine;
  // Since when the router builds a route, by the time of the phone.
  std::optional<std::chrono::steady_clock::time_point> m_waitsSince;
  bool m_timeSet = false;
  int64_t m_seq = 0;
  int64_t m_yawMdeg = 0;
  int64_t m_records = 0;
};

void ReplayLogMessage(base::LogLevel level, base::SrcPoint const & src, std::string const & message)
{
  // The other threads and the errors go where they went.
  if (g_logTo != nullptr && level < base::g_LogAbortLevel && std::this_thread::get_id() == g_replayThread)
    g_logTo->Write(DebugPrint(src) + message);
  else
    g_previousLog(level, src, message);
}

std::unique_ptr<Replay> g_replay;

void RunReplay()
{
  if (!g_replay)
    return;
  if (g_replay->RunChunk())
    GetPlatform().RunTask(Platform::Thread::Gui, &RunReplay);
  else
    g_replay.reset();
}
}  // namespace

extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeReplay(JNIEnv * env, jclass, jstring inputPath,
                                                                    jstring outputPath)
{
  if (g_replay)
    return;
  LOG(LINFO, ("A trip is driven again from", jni::ToNativeString(env, inputPath)));
  g_replay = std::make_unique<Replay>(g_framework->NativeFramework()->GetRoutingManager(),
                                      jni::ToNativeString(env, inputPath), jni::ToNativeString(env, outputPath));
  GetPlatform().RunTask(Platform::Thread::Gui, &RunReplay);
}
}  // extern "C"
