#include "testing/testing.hpp"

#include "map/gps_track.hpp"
#include "map/gps_track_storage.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <chrono>
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
    return [this](IndexedPoints && added, IdRange const & removed, TrackStatistics const &)
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
}  // namespace gps_track_test
