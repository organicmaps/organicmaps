#pragma once

#include "map/map_tests/nogps_test_roads.hpp"

#include "map/nogps/clock.hpp"
#include "map/nogps/delegate.hpp"
#include "map/nogps/esp32_protocol.hpp"
#include "map/nogps/map_api.hpp"
#include "map/nogps/scheduler.hpp"
#include "map/nogps/storage.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <string>
#include <vector>

// Everything around the navigation without GPS for its tests: the time goes on only when the test moves it.
namespace nogps_test
{
class TestClock
  : public nogps::Clock
  , public nogps::Scheduler
{
public:
  // Far from zero, as the time since the boot of a phone.
  static int64_t constexpr kStartMs = 1'000'000'000;
  static int64_t constexpr kUnixOffsetMs = 1'700'000'000'000;

  int64_t NowMs() const override { return m_nowMs; }
  int64_t UnixNowMs() const override { return m_nowMs + kUnixOffsetMs; }

  void Post(int64_t delayMs, Task && task) override
  {
    m_tasks.push_back({m_nowMs + delayMs, m_nextId++, std::move(task)});
  }

  /// Moves the time on, running the tasks that come due on the way in their order.
  void Advance(int64_t ms)
  {
    int64_t const target = m_nowMs + ms;
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
    m_nowMs = target;
  }

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

class TestStorage : public nogps::Storage
{
public:
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

/// Remembers what is asked from the platform.
class TestDelegate : public nogps::Delegate
{
public:
  void StartMotionSensors(bool gyroscope) override
  {
    m_sensorsStarted = true;
    m_gyroscope = gyroscope;
  }
  void StopMotionSensors() override { m_sensorsStarted = false; }
  void Elm327Connect(std::string const & address) override { m_elm327Connects.push_back(address); }
  void Elm327Write(std::string const & data) override { m_elm327Writes.push_back(data); }
  void Elm327Close() override { ++m_elm327Closes; }
  void Esp32BleOpen() override { m_esp32BleOpen = true; }
  void Esp32BleSend(std::string const & line) override { m_esp32BleSent.push_back(line); }
  void Esp32BleSendFirmware(std::string const & piece) override { m_firmwarePieces.push_back(piece); }
  void Esp32BleClose() override { m_esp32BleOpen = false; }
  void OnPosition(nogps::Fix const & fix) override { m_positions.push_back(fix); }
  void OnEvent(nogps::Event event) override { m_events.push_back(event); }

  bool HasEvent(nogps::Event event) const
  {
    return std::find(m_events.begin(), m_events.end(), event) != m_events.end();
  }

  bool m_sensorsStarted = false;
  bool m_gyroscope = false;
  std::vector<std::string> m_elm327Connects;
  std::vector<std::string> m_elm327Writes;
  int m_elm327Closes = 0;
  bool m_esp32BleOpen = false;
  std::vector<std::string> m_esp32BleSent;
  std::vector<std::string> m_firmwarePieces;
  std::vector<nogps::Fix> m_positions;
  std::vector<nogps::Event> m_events;
};

/// The grid of streets without a route.
class TestMap : public nogps::MapApi
{
public:
  bool IsNavigating() const override { return m_navigating; }
  nogps::Roads & GetRoads(bool) override { return m_roads ? *m_roads : m_grid; }
  void RebuildRouteIfOffRoute(nogps::Fix const &, double, std::optional<double>) override { ++m_rebuilds; }
  std::optional<nogps::RoadPoint> ProjectToRoute(ms::LatLon const &, double) override { return {}; }
  std::optional<nogps::RoadWalker::Result> ShiftAlongRoute(ms::LatLon const &, std::optional<double>, double) override
  {
    return {};
  }
  std::optional<nogps::RoadPoint> SnapToMainRoad(ms::LatLon const & position, double radiusM) override
  {
    return GetRoads(false).Snap(position, {}, radiusM);
  }
  std::string GetDistanceLeft() const override { return {}; }
  void ShowCarHeading(double bearingDeg) override { m_carHeading = bearingDeg; }
  void ShowOffRoadCar(std::optional<ms::LatLon> const & position) override
  {
    m_offRoadCar = position;
    ++m_offRoadCarShows;
  }

  bool m_navigating = false;
  std::optional<ms::LatLon> m_offRoadCar;
  int m_offRoadCarShows = 0;
  int m_rebuilds = 0;
  std::optional<double> m_carHeading;
  GridRoads m_grid;
  // Other roads instead of the grid.
  nogps::Roads * m_roads = nullptr;
};

/// A data line of the ESP32 box with the totals since its start.
inline std::string BoxDataLine(int64_t seq, int64_t timeMs, int64_t yawMdeg, int speedKmh, int flags)
{
  char body[160];
  std::snprintf(body, sizeof(body), "NGD,1,%lld,%lld,%lld,0,0,%d,0,%X", static_cast<long long>(seq),
                static_cast<long long>(timeMs), static_cast<long long>(yawMdeg), speedKmh, flags);
  char crc[8];
  std::snprintf(crc, sizeof(crc), "%04X", nogps::esp32::Crc16(body));
  return std::string("$") + body + "*" + crc + "\r\n";
}

inline std::string BoxLine(std::string const & body)
{
  char crc[8];
  std::snprintf(crc, sizeof(crc), "%04X", nogps::esp32::Crc16(body));
  return "$" + body + "*" + crc + "\r\n";
}

/// A GPS position at meters east and north of the grid origin.
inline nogps::Fix GpsAt(TestClock const & clock, double eastM, double northM, std::optional<double> bearingDeg = {},
                        std::optional<double> speedMps = {}, double accuracyM = 5)
{
  nogps::Fix fix;
  fix.m_provider = nogps::Provider::Gps;
  fix.m_position = At(eastM, northM);
  fix.m_accuracyM = accuracyM;
  fix.m_bearingDeg = bearingDeg;
  fix.m_speedMps = speedMps;
  fix.m_timeMs = clock.NowMs();
  fix.m_unixTimeMs = clock.UnixNowMs();
  return fix;
}
}  // namespace nogps_test
