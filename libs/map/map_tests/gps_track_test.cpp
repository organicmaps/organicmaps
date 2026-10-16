#include "testing/testing.hpp"

#include "map/gps_track.hpp"
#include "map/gps_track_storage.hpp"
#include "map/track.hpp"

#include "platform/platform.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace gps_track_test
{
namespace fs = std::filesystem;

using namespace std::chrono_literals;
using Points = std::vector<location::GpsInfo>;
using IndexedPoints = std::vector<std::pair<size_t, location::GpsInfo>>;
using IdRange = std::pair<size_t, size_t>;

double constexpr kDegreesPerMeter = 1.0 / 111320.0;

location::GpsInfo MakePoint(double timestamp, double meters, double speed = 15.0)
{
  location::GpsInfo info;
  info.m_timestamp = timestamp;
  info.m_speed = speed;
  info.m_latitude = 0.0;
  info.m_longitude = meters * kDegreesPerMeter;
  info.m_horizontalAccuracy = 5.0;
  info.m_source = location::EAndroidNative;
  return info;
}

Points MakePoints(size_t count)
{
  Points points;
  for (size_t i = 0; i < count; ++i)
    points.push_back(MakePoint(100.0 + i, 15.0 * i));
  return points;
}

class TrackFile
{
  fs::path const m_directory;

public:
  explicit TrackFile(std::string const & name)
    : m_directory(GetPlatform().TmpPathForFile("gps_track_" + name + "_", ""))
    , m_path((m_directory / "track.bin").string())
  {
    TEST(fs::create_directory(m_directory), (m_directory.string()));
  }
  ~TrackFile()
  {
    std::error_code error;
    fs::remove_all(m_directory, error);
    if (error)
      LOG(LERROR, ("Cannot remove GPS track test directory:", m_directory.string(), error.message()));
  }

  bool Exists() const { return fs::exists(m_path); }
  Points Load() const { return GpsTrackStorage(m_path).Load(); }

  std::string const m_path;
};

class TrackCallback
{
public:
  GpsTrack::TGpsTrackDiffCallback GetCallback()
  {
    return [this](IndexedPoints && added, IdRange const & removed)
    {
      std::lock_guard lg(m_mutex);
      m_added.insert(m_added.end(), added.begin(), added.end());
      if (removed.first != GpsTrack::kInvalidId)
        m_removed.push_back(removed);
      m_cv.notify_one();
    };
  }

  bool WaitForAdded(size_t count)
  {
    std::unique_lock ul(m_mutex);
    return m_cv.wait_for(ul, 5s, [this, count] { return m_added.size() >= count; });
  }

  IndexedPoints GetAdded()
  {
    std::lock_guard lg(m_mutex);
    return m_added;
  }

  std::vector<IdRange> GetRemoved()
  {
    std::lock_guard lg(m_mutex);
    return m_removed;
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_cv;
  IndexedPoints m_added;
  std::vector<IdRange> m_removed;
};

class StatisticsCallback
{
public:
  GpsTrack::TGpsTrackStatisticsCallback GetCallback()
  {
    return [this](TrackStatistics const & trackStatistics)
    {
      std::lock_guard lg(m_mutex);
      m_length = trackStatistics.m_length;
      m_cv.notify_one();
    };
  }

  bool WaitForLength(double expected)
  {
    std::unique_lock ul(m_mutex);
    return m_cv.wait_for(ul, 5s, [this, expected] { return std::fabs(m_length - expected) < 1.0; });
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_cv;
  double m_length = 0;
};

double GetLength(Points const & points)
{
  double length = 0;
  for (size_t i = 1; i < points.size(); ++i)
  {
    length += ms::DistanceOnEarth(ms::LatLon(points[i - 1].m_latitude, points[i - 1].m_longitude),
                                  ms::LatLon(points[i].m_latitude, points[i].m_longitude));
  }
  return length;
}

template <typename Fn>
bool WaitFor(Fn && condition, std::chrono::milliseconds timeout = 5s)
{
  auto const deadline = std::chrono::steady_clock::now() + timeout;
  do
  {
    if (condition())
      return true;
    std::this_thread::sleep_for(1ms);
  }
  while (std::chrono::steady_clock::now() < deadline);
  return condition();
}

Points GetPoints(GpsTrack & track)
{
  Points points;
  track.ForEachPoint([&points](location::GpsInfo const & point, size_t)
  {
    points.push_back(point);
    return true;
  });
  return points;
}

void CheckPoints(Points const & actual, Points const & expected)
{
  TEST_EQUAL(actual.size(), expected.size(), ());
  for (size_t i = 0; i < expected.size(); ++i)
  {
    TEST_EQUAL(actual[i].m_timestamp, expected[i].m_timestamp, (i));
    TEST_EQUAL(actual[i].m_latitude, expected[i].m_latitude, (i));
    TEST_EQUAL(actual[i].m_longitude, expected[i].m_longitude, (i));
    TEST_EQUAL(actual[i].m_speed, expected[i].m_speed, (i));
  }
}

UNIT_TEST(GpsTrack_BatchesBackupWrites)
{
  TrackFile file("batch");
  TrackCallback callback;
  GpsTrack track(file.m_path, {}, 1h);
  track.Start();
  track.SetCallback(callback.GetCallback());

  auto const points = MakePoints(3);
  for (auto const & point : points)
    track.AddPoint(point);

  TEST(callback.WaitForAdded(points.size()), ());
  TEST(!file.Exists(), ("Receiving fixes should not write each point to disk"));

  TEST_EQUAL(track.Finalize(), points.size(), ());
  CheckPoints(file.Load(), points);
  track.Clear();
  TEST_EQUAL(track.Finalize(), 0, ());
  TEST(!file.Exists(), ());
}

UNIT_TEST(GpsTrack_BackupTimerWithoutNewPoints)
{
  TrackFile file("timer");
  GpsTrack track(file.m_path, {}, 20ms);
  track.Start();
  auto const points = MakePoints(3);
  track.AddPoints(points);

  auto const expectedSize = sizeof(uint32_t) + points.size() * (8 * sizeof(double) + sizeof(uint8_t));
  TEST(WaitFor(
           [&]
  {
    std::error_code error;
    auto const size = fs::file_size(file.m_path, error);
    return !error && size == expectedSize;
  }),
       ("Pending fixes must be backed up without another location update"));

  CheckPoints(file.Load(), points);
  TEST_EQUAL(track.Finalize(), points.size(), ());
  CheckPoints(GetPoints(track), points);
}

UNIT_TEST(GpsTrack_RecordsWithoutCallback)
{
  TrackFile file("no_callback");
  GpsTrack track(file.m_path);
  track.Start();
  auto const points = MakePoints(5);
  track.AddPoints(points);

  TEST_EQUAL(track.Finalize(), points.size(), ());
  TEST(!track.IsEmpty(), ());
  CheckPoints(GetPoints(track), points);
  CheckPoints(file.Load(), points);
}

UNIT_TEST(GpsTrack_StartRestoresBackupOnce)
{
  TrackFile file("restore");
  auto points = MakePoints(5);
  {
    GpsTrack track(file.m_path);
    track.Start();
    track.AddPoints(points);
  }
  CheckPoints(file.Load(), points);

  TrackCallback callback;
  GpsTrack track(file.m_path);
  track.SetCallback(callback.GetCallback());
  TEST_EQUAL(track.Finalize(), 0, ("Attaching a renderer must not restore a stopped recording"));
  TEST(track.IsEmpty(), ());

  track.Start();
  track.Start();
  points.push_back(MakePoint(110.0, 150.0));
  track.AddPoint(points.back());
  TEST_EQUAL(track.Finalize(), points.size(), ());
  CheckPoints(GetPoints(track), points);
  CheckPoints(file.Load(), points);
}

UNIT_TEST(GpsTrack_FinalizePreservesBackupAndRejectsLatePoints)
{
  TrackFile file("finalize");
  auto points = MakePoints(3);
  TEST(GpsTrackStorage(file.m_path).Append(points), ());
  // This fix is too close for the filter, but must become the saved endpoint on Finalize.
  points.push_back(MakePoint(160.0, 35.0, 0.0));
  {
    GpsTrack track(file.m_path, std::make_unique<GpsTrackFilter>(), 1h);
    track.Start();
    track.AddPoint(points.back());
    TEST_EQUAL(track.Finalize(), points.size(), ());
    CheckPoints(GetPoints(track), points);
    CheckPoints(file.Load(), points);

    track.AddPoint(MakePoint(200.0, 200.0));
    track.AddPoints(MakePoints(2));
    TEST_EQUAL(track.Finalize(), points.size(), ());
    CheckPoints(GetPoints(track), points);
    CheckPoints(file.Load(), points);
  }

  GpsTrack restored(file.m_path, std::make_unique<GpsTrackFilter>(), 1h);
  restored.Start();
  TEST_EQUAL(restored.Finalize(), points.size(), ());
  CheckPoints(GetPoints(restored), points);
  CheckPoints(file.Load(), points);

  restored.Clear();
  TEST_EQUAL(restored.Finalize(), 0, ());
  TEST(restored.IsEmpty(), ());
  TEST(!file.Exists(), ("Discard must remove the backup after finalizing"));

  // Discarding resets the filter and collection, including their timestamp history.
  restored.Start();
  auto const nextPoints = MakePoints(2);
  restored.AddPoints(nextPoints);
  TEST_EQUAL(restored.Finalize(), nextPoints.size(), ());
  CheckPoints(GetPoints(restored), nextPoints);
  CheckPoints(file.Load(), nextPoints);
}

UNIT_TEST(GpsTrack_RestoresRawFilterHistory)
{
  TrackFile file("filter_history");
  auto prefix = MakePoints(3);
  auto turn = MakePoint(103.0, 30.0);
  turn.m_latitude = 5.0 * kDegreesPerMeter;
  prefix.push_back(turn);
  Points const suffix = {MakePoint(104.0, 45.0), MakePoint(105.0, 60.0), MakePoint(160.0, 65.0, 0.0)};

  // The rejected turn affects acceptance of the next fix; the last close fix is appended by Finalize.
  GpsTrackFilter filter;
  Points expected;
  filter.Process(prefix, expected);
  filter.Process(suffix, expected);
  filter.Finalize(expected);
  TEST_EQUAL(expected.size(), 5, ());
  {
    GpsTrack track(file.m_path, std::make_unique<GpsTrackFilter>());
    track.Start();
    track.AddPoints(prefix);
  }
  CheckPoints(file.Load(), prefix);

  GpsTrack restored(file.m_path, std::make_unique<GpsTrackFilter>());
  restored.Start();
  restored.AddPoints(suffix);
  TEST_EQUAL(restored.Finalize(), expected.size(), ());
  CheckPoints(GetPoints(restored), expected);
  auto rawPoints = prefix;
  rawPoints.insert(rawPoints.end(), suffix.begin(), suffix.end());
  CheckPoints(file.Load(), rawPoints);
}

UNIT_TEST(GpsTrack_StartReplacesUnreadableBackup)
{
  base::ScopedLogAbortLevelChanger const expectedError(LCRITICAL);
  TrackFile file("unreadable");
  size_t const headerSizes[] = {0, 1, sizeof(uint32_t)};
  auto const points = MakePoints(3);
  for (auto const headerSize : headerSizes)
  {
    {
      std::ofstream writer(file.m_path, std::ios::binary | std::ios::trunc);
      uint32_t const unsupportedVersion = 0;
      writer.write(reinterpret_cast<char const *>(&unsupportedVersion), headerSize);
      writer.close();
      TEST(writer.good(), ());
    }

    TrackCallback callback;
    GpsTrack track(file.m_path, {}, 1h);
    track.Start();
    track.SetCallback(callback.GetCallback());
    track.AddPoints(points);
    TEST(callback.WaitForAdded(points.size()), (headerSize));
    TEST(!file.Exists(), ("Unreadable backup must be removed before a new batch", headerSize));
    TEST_EQUAL(track.Finalize(), points.size(), (headerSize));
    CheckPoints(GetPoints(track), points);
    CheckPoints(file.Load(), points);
    track.Clear();
    TEST_EQUAL(track.Finalize(), 0, (headerSize));
  }
}

UNIT_TEST(GpsTrack_ClearResetsBackupAndKeepsIdsMonotonic)
{
  TrackFile file("clear");
  auto const oldPoints = MakePoints(3);
  Points const newPoints = {MakePoint(10.0, 0.0), MakePoint(11.0, 15.0)};
  {
    GpsTrack track(file.m_path);
    track.Start();
    track.AddPoints(oldPoints);
  }
  TrackCallback callback;
  {
    GpsTrack track(file.m_path, std::make_unique<GpsTrackFilter>(), 1h);
    track.Start();
    track.SetCallback(callback.GetCallback());
    TEST(callback.WaitForAdded(oldPoints.size()), ());
    TEST(file.Exists(), ());

    track.Clear();
    track.AddPoints(newPoints);
    TEST(callback.WaitForAdded(oldPoints.size() + newPoints.size()), ());
    TEST(!file.Exists(), ("Clear must delete the old backup before the next batch"));

    auto const added = callback.GetAdded();
    TEST_EQUAL(added.size(), oldPoints.size() + newPoints.size(), ());
    for (size_t i = 0; i < added.size(); ++i)
      TEST_EQUAL(added[i].first, i, ());
    auto const removed = callback.GetRemoved();
    TEST(std::find(removed.begin(), removed.end(), IdRange(0, oldPoints.size() - 1)) != removed.end(), ());
  }

  CheckPoints(file.Load(), newPoints);
  GpsTrack restored(file.m_path);
  restored.Start();
  TEST_EQUAL(restored.Finalize(), newPoints.size(), ());
  CheckPoints(GetPoints(restored), newPoints);
}

UNIT_TEST(GpsTrack_StartKeepsPendingClear)
{
  TrackFile file("clear_start");
  GpsTrack track(file.m_path, std::make_unique<GpsTrackFilter>(), 1h);
  auto const oldPoints = MakePoints(3);
  track.Start();
  track.AddPoints(oldPoints);
  TEST_EQUAL(track.Finalize(), oldPoints.size(), ());
  CheckPoints(file.Load(), oldPoints);

  track.Clear();
  track.Start();
  TEST_EQUAL(track.Finalize(), 0, ("Restarting must not restore a backup pending deletion"));
  TEST(!file.Exists(), ());

  Points const newPoints = {MakePoint(10.0, 0.0), MakePoint(11.0, 15.0)};
  track.Start();
  track.AddPoints(newPoints);
  TEST_EQUAL(track.Finalize(), newPoints.size(), ());
  CheckPoints(GetPoints(track), newPoints);
  CheckPoints(file.Load(), newPoints);
}

UNIT_TEST(GpsTrack_StatisticsWithoutDiffCallback)
{
  TrackFile file("statistics");
  StatisticsCallback callback;
  GpsTrack track(file.m_path);
  track.Start();
  track.SetStatisticsCallback(callback.GetCallback());

  auto const points = MakePoints(5);
  track.AddPoints(points);

  TEST(callback.WaitForLength(GetLength(points)), ());
  TEST_ALMOST_EQUAL_ABS(track.GetTrackStatistics().m_length, GetLength(points), 1.0, ());
}

UNIT_TEST(GpsTrack_StatisticsForNewCallback)
{
  TrackFile file("statistics_new_callback");
  GpsTrack track(file.m_path);
  track.Start();

  StatisticsCallback first;
  track.SetStatisticsCallback(first.GetCallback());
  auto const points = MakePoints(5);
  track.AddPoints(points);
  TEST(first.WaitForLength(GetLength(points)), ());

  StatisticsCallback second;
  track.SetStatisticsCallback(second.GetCallback());
  TEST(second.WaitForLength(GetLength(points)), ());
}

// Reported right away, without waiting for new points.
UNIT_TEST(GpsTrack_StatisticsOfRestoredTrack)
{
  TrackFile file("statistics_restored");
  auto const points = MakePoints(5);
  {
    GpsTrack track(file.m_path);
    track.Start();
    track.AddPoints(points);
  }

  StatisticsCallback callback;
  GpsTrack track(file.m_path);
  track.Start();
  track.SetStatisticsCallback(callback.GetCallback());

  TEST(callback.WaitForLength(GetLength(points)), ());
  TEST(!track.IsEmpty(), ());
}

UNIT_TEST(GpsTrack_LargeBackupRoundTrip)
{
  TrackFile file("large");
  auto const points = MakePoints(50000);
  {
    GpsTrack track(file.m_path);
    track.Start();
    track.AddPoints(points);
  }
  GpsTrack restored(file.m_path);
  restored.Start();
  TEST_EQUAL(restored.Finalize(), points.size(), ());
  CheckPoints(GetPoints(restored), points);
}
UNIT_TEST(GpsTrack_ElevationSnapshots)
{
  TrackFile file("elevation");

  auto makePoint = [](size_t index, double altitude)
  {
    auto point = MakePoint(1000000 + index * 20, index * 30.0);
    point.m_altitude = altitude;
    point.m_verticalAccuracy = 1.0;
    return point;
  };

  TrackCallback callback;
  GpsTrack track(file.m_path);
  track.Start();
  track.SetCallback(callback.GetCallback());
  track.AddPoints({makePoint(0, 100), makePoint(1, 100), makePoint(2, 150)});
  TEST(callback.WaitForAdded(3), ());

  auto const snapshot = track.GetElevationSnapshot();
  TEST(snapshot, ());
  TEST_EQUAL(snapshot->m_elevation.GetLines().front().back().m_altitude, 150, ());

  track.AddPoints({makePoint(3, 100), makePoint(4, 100)});
  TEST(callback.WaitForAdded(5), ());
  auto const callbackStatistics = track.GetTrackStatistics();
  TEST_EQUAL(callbackStatistics.m_ascent, 0, ());
  TEST_EQUAL(callbackStatistics.m_descent, 0, ());
  TEST_EQUAL(callbackStatistics.m_maxElevation, 100, ());
  TEST_EQUAL(track.GetElevationSnapshot()->m_elevation.GetSize(), 2, ());
  // The worker may replace its cached profile while a UI consumer holds an older snapshot.
  TEST_EQUAL(snapshot->m_elevation.GetLines().front().back().m_altitude, 150, ());

  TEST_EQUAL(track.Finalize(), 5, ());
  kml::TrackData data;
  data.m_id = 42;
  data.m_layers.emplace_back();
  data.m_geometry.m_lines.emplace_back();
  data.m_geometry.m_timestamps.emplace_back();
  track.ForEachPoint([&](location::GpsInfo const & point, size_t)
  {
    data.m_geometry.m_lines.back().emplace_back(mercator::FromLatLon(point.m_latitude, point.m_longitude),
                                                static_cast<geometry::Altitude>(point.m_altitude));
    data.m_geometry.m_timestamps.back().push_back(point.m_timestamp);
    return true;
  });
  auto const saved = Track(std::move(data)).GetStatistics();
  TEST_EQUAL(callbackStatistics.m_ascent, saved.m_ascent, ());
  TEST_EQUAL(callbackStatistics.m_descent, saved.m_descent, ());
  TEST_EQUAL(callbackStatistics.m_minElevation, saved.m_minElevation, ());
  TEST_EQUAL(callbackStatistics.m_maxElevation, saved.m_maxElevation, ());
  TEST_EQUAL(callbackStatistics.m_length, saved.m_length, ());
  TEST_EQUAL(callbackStatistics.m_duration, saved.m_duration, ());
}
UNIT_TEST(GpsTrack_ElevationCacheCancellationAndReset)
{
  TrackFile file("elevation_cache");
  TrackCallback callback;
  GpsTrack track(file.m_path);
  track.Start();
  // A renderer can consume the raw diff without requesting altitude statistics.
  track.SetCallback(callback.GetCallback());
  auto point = [](size_t index, int altitude)
  {
    auto gps = MakePoint(1000000 + index * 20, index * 30.0);
    gps.m_altitude = altitude;
    return gps;
  };
  std::vector<location::GpsInfo> points;
  for (size_t i = 0; i < 700; ++i)
    points.push_back(point(i, 100 + i % 50));
  track.AddPoints(points);
  TEST(callback.WaitForAdded(700), ());
  auto const first = track.GetElevationSnapshot();
  TEST(first, ());
  auto checkProfile = [&](GpsTrack::ElevationSnapshot const & snapshot)
  {
    kml::TrackData data;
    data.m_id = 42;
    data.m_layers.emplace_back();
    data.m_geometry.m_lines.emplace_back();
    data.m_geometry.m_timestamps.emplace_back();
    for (auto const & gps : points)
    {
      data.m_geometry.m_lines.back().emplace_back(mercator::FromLatLon(gps.m_latitude, gps.m_longitude),
                                                  static_cast<geometry::Altitude>(gps.m_altitude));
      data.m_geometry.m_timestamps.back().push_back(gps.m_timestamp);
    }
    Track saved(std::move(data));
    auto const & expected = saved.GetElevationInfo()->GetLines().front();
    auto const & actual = snapshot.m_elevation.GetLines().front();
    TEST_EQUAL(actual.size(), expected.size(), ());
    for (size_t i = 0; i < std::min(actual.size(), expected.size()); ++i)
    {
      TEST_EQUAL(actual[i].m_altitude, expected[i].m_altitude, (i));
      TEST_EQUAL(actual[i].m_distance, expected[i].m_distance, (i));
    }
  };
  checkProfile(*first);
  TEST_EQUAL(first->m_revision, points.size(), ());
  TEST_EQUAL(first, track.GetElevationSnapshot(), ());
  // Cancellation callbacks are invoked outside the recording lock, including cache hits.
  TEST_EQUAL(first,
             track.GetElevationSnapshot(
                 [&]
  {
    TEST_EQUAL(track.GetTrackStatistics().m_ascent, first->m_statistics.m_ascent, ());
    return false;
  }),
             ());

  points.push_back(point(700, 125));
  track.AddPoint(points.back());
  TEST(callback.WaitForAdded(701), ());
  size_t calls = 0;
  TEST(!track.GetElevationSnapshot([&] { return ++calls == 4; }), ());
  auto const next = track.GetElevationSnapshot();
  checkProfile(*next);
  TEST_EQUAL(next->m_revision, 701, ());
  TEST_EQUAL(first->m_revision, 700, ());
  TEST_EQUAL(next->m_statistics.m_ascent, track.GetTrackStatistics().m_ascent, ());

  track.Clear();
  TEST(!track.GetElevationSnapshot(), ());

  track.AddPoints({point(0, 300), point(1, 311), point(2, 300)});
  TEST(callback.WaitForAdded(704), ());
  auto const reset = track.GetElevationSnapshot();
  TEST_NOT_EQUAL(reset->m_epoch, first->m_epoch, ());
  TEST_EQUAL(reset->m_revision, 3, ());
  TEST_EQUAL(reset->m_statistics.m_ascent, 11, ());
  TEST_EQUAL(reset->m_statistics.m_descent, 11, ());
  TEST_EQUAL(first->m_revision, 700, ());
}
UNIT_TEST(GpsTrack_ElevationCacheRestartsWithUnfinalizedBackup)
{
  TrackFile file("elevation_restart");
  GpsTrack track(file.m_path, std::make_unique<GpsTrackFilter>());
  auto points = MakePoints(3);
  points.push_back(MakePoint(160.0, 35.0, 0.0));
  for (auto & point : points)
    point.m_altitude = 100;
  track.Start();
  track.AddPoints(points);
  TEST_EQUAL(track.Finalize(), points.size(), ());
  auto const finalized = track.GetElevationSnapshot();
  TEST(finalized, ());
  TEST_EQUAL(finalized->m_revision, points.size(), ());

  // Restore replays raw fixes, so the close endpoint is provisional again until Finalize.
  track.Start();
  std::shared_ptr<GpsTrack::ElevationSnapshot const> restored;
  TEST(WaitFor(
           [&]
  {
    restored = track.GetElevationSnapshot();
    return restored && restored->m_epoch != finalized->m_epoch;
  }),
       ());
  TEST_EQUAL(restored->m_revision, points.size() - 1, ());
  TEST_EQUAL(finalized->m_revision, points.size(), ());
  TEST_EQUAL(track.Finalize(), points.size(), ());
}
}  // namespace gps_track_test
