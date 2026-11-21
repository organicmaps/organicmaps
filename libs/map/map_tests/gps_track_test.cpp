#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/gps_track.hpp"
#include "map/gps_track_storage.hpp"
#include "map/gps_tracker.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"

#include "platform/platform.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "defines.hpp"

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
  info.m_verticalAccuracy = 1.0;
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
    TEST_EQUAL(actual[i].HasAltitude(), expected[i].HasAltitude(), (i));
    TEST_EQUAL(actual[i].m_altitude, expected[i].m_altitude, (i));
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
  CheckPoints(GetPoints(track), points);
  CheckPoints(file.Load(), points);
}

using df::test_support::VisualParamsFixture;
using SaveResult = Framework::TrackRecordingSaveResult;

UNIT_CLASS_TEST(VisualParamsFixture, Framework_SaveRecordingFinalizesBeforeCheckingGeometry)
{
  Framework framework({}, false /* loadMaps */);
  auto & bookmarks = framework.GetBookmarkManager();
  bookmarks.EnableTestMode(true);
  auto & tracker = GpsTracker::Instance();
  tracker.Clear();
  SCOPE_GUARD(cleanup, [&]
  {
    framework.DiscardTrackRecording();
    tracker.Stop();
  });

  auto start = MakePoint(100.0, 0.0);
  start.m_longitude = 180.0;
  start.m_altitude = 100.0;
  auto finish = MakePoint(160.0, 0.0);
  finish.m_longitude = -179.999999;
  framework.StartTrackRecording();
  tracker.OnLocationUpdated(start);
  tracker.OnLocationUpdated(finish);
  TEST(framework.SaveTrackRecordingWithName("Stationary") == SaveResult::Empty, ());

  finish.m_longitude = -179.99998;
  finish.m_altitude = 102.0;
  auto missingAltitude = finish;
  missingAltitude.m_verticalAccuracy = -1.0;
  framework.StartTrackRecording();
  tracker.OnLocationUpdated(start);
  tracker.OnLocationUpdated(missingAltitude);
  TEST(framework.SaveTrackRecordingWithName("Missing altitude") == SaveResult::Success, ());
  auto const * incomplete = bookmarks.GetTrack(framework.GetCurrentPlacePageInfo().GetTrackId());
  TEST(incomplete, ());
  TEST(!incomplete->GetElevationInfo(), ());
  auto const & mixedGeometry = incomplete->GetData().m_geometry;
  TEST_EQUAL(mixedGeometry.m_lines.front(),
             (kml::TrackGeometry{{{180.0, 0}, 100}, {{-179.99998, 0}, geometry::kInvalidAltitude}}), ());
  TEST_EQUAL(mixedGeometry.m_timestamps.front(), (kml::MultiGeometry::TimeT{100, 160}), ());
  TEST_EQUAL(incomplete->GetDurationInSeconds(), 60.0, ());

  // The sampling filter defers this close but distinct fix until finalization.
  framework.StartTrackRecording();
  tracker.OnLocationUpdated(start);
  tracker.OnLocationUpdated(finish);
  TEST(framework.SaveTrackRecordingWithName("Recorded track") == SaveResult::Success, ());
  TEST(!framework.IsTrackRecordingEnabled(), ());

  auto const id = framework.GetCurrentPlacePageInfo().GetTrackId();
  auto const * track = bookmarks.GetTrack(id);
  TEST(track, ());
  TEST(track->GetElevationInfo(), ());
  auto const & geometry = track->GetData().m_geometry;
  TEST_EQUAL(geometry.m_lines.front().size(), 2, ());
  TEST_EQUAL(geometry.m_lines.front().back().GetAltitude(), 102, ());
  TEST_EQUAL(geometry.m_timestamps.front(), (kml::MultiGeometry::TimeT{100, 160}), ());

  start.m_altitude = finish.m_altitude = 0;
  framework.StartTrackRecording();
  tracker.OnLocationUpdated(start);
  tracker.OnLocationUpdated(finish);
  TEST(framework.SaveTrackRecordingWithName("Sea level") == SaveResult::Success, ());
  auto const * seaLevel = bookmarks.GetTrack(framework.GetCurrentPlacePageInfo().GetTrackId());
  TEST(seaLevel, ());
  for (auto const & point : seaLevel->GetData().m_geometry.m_lines.front())
    TEST_EQUAL(point.GetAltitude(), 0, ());
  TEST(!seaLevel->GetElevationInfo(), ());
}

UNIT_CLASS_TEST(VisualParamsFixture, Framework_RetainsRecordingWhenSaveFails)
{
  auto & platform = GetPlatform();
  auto const originalSettings = platform.SettingsDir();
  auto const settings = platform.TmpPathForFile("recording_save_", "");
  TEST(fs::create_directory(settings), (settings));
  SCOPE_GUARD(restoreSettings, [&]
  {
    platform.SetSettingsDir(originalSettings);
    std::error_code error;
    fs::remove_all(settings, error);
    if (error)
      LOG(LERROR, ("Cannot remove recording test settings:", error.message()));
  });
  platform.SetSettingsDir(settings);

  Framework framework({}, false /* loadMaps */);
  auto & bookmarks = framework.GetBookmarkManager();
  bookmarks.EnableTestMode(true);
  auto & tracker = GpsTracker::Instance();
  tracker.Clear();
  SCOPE_GUARD(clearRecording, [&]
  {
    framework.DiscardTrackRecording();
    tracker.Stop();
  });

  auto destination = std::make_unique<kml::FileData>();
  kml::SetDefaultStr(destination->m_categoryData.m_name, "Test recording");
  destination->m_categoryData.m_lastModified = kml::TimestampClock::from_time_t(1);
  destination->m_bookmarksData.emplace_back();
  BookmarkManager::KMLDataCollection loadedLists;
  loadedLists.emplace_back("", std::move(destination));
  bookmarks.CreateCategories(std::move(loadedLists), true /* autoSave */);
  auto const groupId = bookmarks.GetUnsortedBmGroupsIdList().front();
  auto const originalMarks = bookmarks.GetUserMarkIds(groupId);
  TEST_EQUAL(originalMarks.size(), 1, ());
  TEST(bookmarks.SaveBookmarkCategory(groupId), ());
  bookmarks.SetLastEditedBmCategory(groupId);

  kml::CategoryData otherList;
  kml::SetDefaultStr(otherList.m_name, "Initially more recent");
  otherList.m_lastModified = kml::TimestampClock::from_time_t(2);
  auto const initiallyRecent = bookmarks.CreateBookmarkCategory(std::move(otherList));
  TEST_EQUAL(bookmarks.GetSortedBmGroupIdList().front(), initiallyRecent, ());

  auto const categoryPath = bookmarks.GetCategoryFileName(groupId);
  TEST(fs::remove(categoryPath), (categoryPath));
  TEST(fs::create_directory(categoryPath), (categoryPath));

  framework.StartTrackRecording();
  tracker.OnLocationUpdated(MakePoint(100.0, 0.0));
  tracker.OnLocationUpdated(MakePoint(160.0, 15.0));

  auto const blockedParent = fs::path(settings) / "obstruction";
  TEST(std::ofstream(blockedParent).good(), (blockedParent.string()));
  {
    SCOPE_GUARD(restoreDestination, [&] { platform.SetSettingsDir(settings); });
    platform.SetSettingsDir((blockedParent / "settings").string());
    TEST(framework.SaveTrackRecordingWithName("Recorded track") == SaveResult::FileError, ());
  }
  TEST(framework.IsTrackRecordingEnabled(), ());

  TEST(framework.SaveTrackRecordingWithName("Recorded track") == SaveResult::FileError, ());
  auto const backupPath = base::JoinPath(platform.WritableDir(), GPS_TRACK_FILENAME);
  TEST(framework.IsTrackRecordingEnabled(), ());
  TEST_EQUAL(GpsTrackStorage(backupPath).Load().size(), 2, ());
  TEST(bookmarks.GetTrackIds(groupId).empty(), ());
  TEST_EQUAL(bookmarks.GetUserMarkIds(groupId), originalMarks, ());
  TEST_EQUAL(bookmarks.GetSortedBmGroupIdList().front(), initiallyRecent, ());

  TEST(fs::remove(categoryPath), (categoryPath));
  auto const selection = *originalMarks.begin();
  framework.ShowBookmark(selection);
  TEST(framework.SaveTrackRecordingWithName("Recorded track", false /* showTrack */) == SaveResult::Success, ());
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetBookmarkId(), selection, ());
  TEST(!framework.IsTrackRecordingEnabled(), ());
  TEST_EQUAL(bookmarks.GetTrackIds(groupId).size(), 1, ());
  auto persisted = LoadKmlFile(categoryPath, FileType::Kml);
  TEST(persisted, (categoryPath));
  TEST_EQUAL(persisted->m_bookmarksData.size(), 1, ());
  TEST_EQUAL(persisted->m_tracksData.size(), 1, ());
  TEST_EQUAL(persisted->m_tracksData.front().m_geometry.m_timestamps.front(), (kml::MultiGeometry::TimeT{100, 160}),
             ());
  TEST_EQUAL(bookmarks.GetSortedBmGroupIdList().front(), groupId, ());
  BookmarkManager::KMLDataCollection reloadedLists;
  reloadedLists.emplace_back(categoryPath, std::move(persisted));
  bookmarks.CreateCategories(std::move(reloadedLists), true /* autoSave */);
  TEST_EQUAL(bookmarks.GetSortedBmGroupIdList().front(), groupId, ("Reloaded category stays the most recent"));

  // Join the track thread so the asynchronous backup cleanup has finished.
  tracker.Stop();
  TEST(!fs::exists(backupPath), (backupPath));
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
    TrackCallback live;
    GpsTrack track(file.m_path, std::make_unique<GpsTrackFilter>(), 1h);
    track.SetCallback(live.GetCallback());
    track.Start();
    track.AddPoint(points.back());
    TEST_EQUAL(track.Finalize(), points.size(), ());
    CheckPoints(GetPoints(track), points);
    CheckPoints(file.Load(), points);
    // A failed save can resume this recording with the same render sink.
    auto const added = live.GetAdded();
    TEST_EQUAL(added.size(), points.size(), ());
    TEST_EQUAL(added.back().second.m_timestamp, points.back().m_timestamp, ());

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
  prefix[1].m_verticalAccuracy = -1.0;
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
  TEST_EQUAL(expected.front().m_timestamp, 100.0, ());
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
  TEST(restored.GetElevationInfo().IsEmpty(), ());
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
  CheckPoints(GetPoints(track), points);
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
