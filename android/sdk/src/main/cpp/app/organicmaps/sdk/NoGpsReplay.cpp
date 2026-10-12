#include "app/organicmaps/sdk/Framework.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/nogps/clock.hpp"
#include "map/nogps/delegate.hpp"
#include "map/nogps/engine.hpp"
#include "map/nogps/esp32_protocol.hpp"
#include "map/nogps/geo.hpp"
#include "map/nogps/map_api.hpp"
#include "map/nogps/scheduler.hpp"
#include "map/nogps/service.hpp"
#include "map/nogps/storage.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// A recorded trip driven again, for debug builds: everything the navigation without GPS was given on a trip, taken
// from its trip log, goes through a navigation of its own on the roads of the map, as fast as the phone counts, and
// what it logs is written to a file as a trip log. It tells what a new rule does on the trips already driven, where
// GPS was seen, before the user drives with it. The route of the trip is not driven again: there are only roads.
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
//                                 the reverse button, the pause button pressed to resume or to pause, the mark
//                                 moved along the road
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
// back to them.
class ReplayStorage : public Storage
{
public:
  ReplayStorage()
  {
    SettingsStorage const settings;
    for (auto const key : {kSpeedScale, kSpeedTable, kSpeedLag, kBoxAccel})
      if (auto const value = settings.GetString(key))
        m_values[std::string(key)] = *value;
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
  void StartMotionSensors(bool) override {}
  void StopMotionSensors() override {}
  void Elm327Connect(std::string const &) override {}
  void Elm327Write(std::string const &) override {}
  void Elm327Close() override {}
  void Esp32BleOpen() override {}
  void Esp32BleSend(std::string const &) override {}
  void Esp32BleSendFirmware(std::string const &) override {}
  void Esp32BleClose() override {}
  void OnPosition(Fix const &) override {}
  void OnEvent(Event event) override { LOG(LINFO, ("EVENT", DebugPrint(event))); }
};

// The roads of the map without the route of the user, nothing is shown on the map.
class ReplayMap : public MapApi
{
public:
  ReplayMap(RoutingManager & routing, Clock const & clock)
    : m_map(routing, clock, [](double) {}, [](std::optional<ms::LatLon> const &) {})
  {}

  bool IsNavigating() const override { return false; }
  Roads & GetRoads(bool) override { return m_map.GetRoads(false /* matchRoute */); }
  void RebuildRouteIfOffRoute(Fix const &, double, std::optional<double>) override {}
  std::optional<RoadPoint> ProjectToRoute(ms::LatLon const &, double) override { return {}; }
  std::optional<RoadWalker::Result> ShiftAlongRoute(ms::LatLon const &, std::optional<double>, double) override
  {
    return {};
  }
  std::optional<RoadPoint> SnapToMainRoad(ms::LatLon const & position, double radiusM) override
  {
    return m_map.SnapToMainRoad(position, radiusM);
  }
  std::string GetDistanceLeft() const override { return {}; }
  void ShowCarHeading(double) override {}
  void ShowOffRoadCar(std::optional<ms::LatLon> const &) override {}

private:
  RoutingMapApi m_map;
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
    , m_map(routing, m_clock)
  {
    std::remove(m_donePath.c_str());
    NewEngine();
  }

  // Drives on for a part of a second of the phone. \returns false when the trip is over.
  bool RunChunk()
  {
    g_logTo = this;
    g_replayThread = std::this_thread::get_id();
    g_previousLog = base::SetLogMessageFn(&ReplayLogMessage);
    auto const started = std::chrono::steady_clock::now();
    bool more = true;
    std::string line;
    while (std::chrono::steady_clock::now() - started < std::chrono::milliseconds(120))
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
        NewEngine();
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
    case 'P':
    {
      // The button was pressed to pause or to resume. The pause may be over by itself here, or never set.
      int paused = 0;
      if ((in >> paused) && engine.GetStatus().m_paused != (paused != 0))
        engine.TogglePause();
      break;
    }
    case 'M':
    {
      double meters = 0;
      if ((in >> meters) && engine.IsManualMode())
        engine.ShiftPosition(meters);
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
      int const length = std::snprintf(body, sizeof(body), "NGD,1,%lld,%lld,%lld,0,0,%d,0,%s",
                                       static_cast<long long>(++m_seq), static_cast<long long>(boxMs),
                                       static_cast<long long>(startMdeg + yawMdeg * i / kLines), speedKmh,
                                       flags.c_str());
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
  ReplayDelegate m_delegate;
  ReplayMap m_map;
  std::unique_ptr<Engine> m_engine;
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
