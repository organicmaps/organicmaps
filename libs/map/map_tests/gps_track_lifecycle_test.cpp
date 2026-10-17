#include "testing/testing.hpp"

#include "map/gps_track.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <vector>

namespace gps_track_lifecycle_test
{
namespace
{
auto GetFilePath()
{
  return base::JoinPath(GetPlatform().WritableDir(), "gps_track_lifecycle.bin");
}

std::vector<location::GpsInfo> MakePoints()
{
  std::vector<location::GpsInfo> points;
  for (size_t i = 0; i < 5; ++i)
  {
    location::GpsInfo point;
    point.m_timestamp = 1000 + 20 * i;
    point.m_latitude = 45 + 0.0003 * i;
    point.m_longitude = 10;
    point.m_speed = 2;
    point.m_horizontalAccuracy = 3;
    point.m_altitude = 100 + i;
    point.m_verticalAccuracy = 1;
    points.push_back(point);
  }
  return points;
}

void CheckPoints(GpsTrack & track, std::vector<location::GpsInfo> const & expected)
{
  TEST_EQUAL(track.Finalize(), expected.size(), ());
  size_t index = 0;
  track.ForEachPoint([&](location::GpsInfo const & point, size_t)
  {
    TEST_LESS(index, expected.size(), ());
    TEST_EQUAL(point.m_timestamp, expected[index].m_timestamp, ());
    TEST_EQUAL(point.m_latitude, expected[index].m_latitude, ());
    TEST_EQUAL(point.m_altitude, expected[index].m_altitude, ());
    ++index;
    return true;
  });
  TEST_EQUAL(index, expected.size(), ());
}
}  // namespace

UNIT_TEST(GpsTrack_RecordAndRestoreWithoutObservers)
{
  auto const filePath = GetFilePath();
  FileWriter::DeleteFileX(filePath);
  SCOPE_GUARD(deleteFile, [&] { FileWriter::DeleteFileX(filePath); });
  auto const points = MakePoints();
  TrackStatistics statistics;
  {
    GpsTrack track(filePath, std::make_unique<GpsTrackFilter>());
    track.AddPoints(points);
    TEST(!track.IsEmpty(), ());
    CheckPoints(track, points);
    statistics = track.GetTrackStatistics();
    TEST_GREATER(statistics.m_length, 100, ());
  }
  {
    GpsTrack restored(filePath, std::make_unique<GpsTrackFilter>());
    restored.Restore();
    TEST(!restored.IsEmpty(), ());
    CheckPoints(restored, points);
    TEST_ALMOST_EQUAL_ABS(restored.GetTrackStatistics().m_length, statistics.m_length, 1e-6, ());
    TEST_EQUAL(restored.GetTrackStatistics().m_duration, statistics.m_duration, ());
  }
}

UNIT_TEST(GpsTrack_FinalizeRestoresWithoutStartingWorker)
{
  auto const filePath = GetFilePath();
  FileWriter::DeleteFileX(filePath);
  SCOPE_GUARD(deleteFile, [&] { FileWriter::DeleteFileX(filePath); });
  auto const points = MakePoints();
  {
    GpsTrackStorage storage(filePath);
    storage.Append(points);
  }
  GpsTrack restored(filePath, std::make_unique<GpsTrackFilter>());
  CheckPoints(restored, points);
}

UNIT_TEST(GpsTrack_DisconnectingRendererDoesNotStartRecording)
{
  auto const filePath = GetFilePath();
  FileWriter::DeleteFileX(filePath);
  SCOPE_GUARD(deleteFile, [&] { FileWriter::DeleteFileX(filePath); });
  {
    GpsTrack track(filePath);
    track.SetCallback(nullptr);
  }
  TEST(!Platform::IsFileExistsByFullPath(filePath), ());
}

UNIT_TEST(GpsTrack_EmptyDecisionDrainsQueuedPoints)
{
  auto const filePath = GetFilePath();
  FileWriter::DeleteFileX(filePath);
  SCOPE_GUARD(deleteFile, [&] { FileWriter::DeleteFileX(filePath); });
  GpsTrack track(filePath);
  TEST(track.IsEmpty(), ());
  auto const points = MakePoints();
  track.AddPoint(points[0]);
  TEST(track.IsEmpty(), ());
  track.AddPoint(points[1]);
  TEST(!track.IsEmpty(), ());
  track.Clear();
  TEST(track.IsEmpty(), ());
  TEST_EQUAL(track.GetTrackStatistics().m_length, 0, ());
}

UNIT_TEST(GpsTrack_ConcurrentEmptyChecksDrainQueuedPoints)
{
  auto const filePath = GetFilePath();
  FileWriter::DeleteFileX(filePath);
  SCOPE_GUARD(deleteFile, [&] { FileWriter::DeleteFileX(filePath); });
  GpsTrack track(filePath);
  auto const check = [&]
  {
    for (size_t i = 0; i < 100; ++i)
      track.IsEmpty();
  };
  std::thread first(check);
  std::thread second(check);
  track.AddPoints(MakePoints());
  first.join();
  second.join();
  TEST(!track.IsEmpty(), ());
  TEST_EQUAL(track.Finalize(), 5, ());
}

UNIT_TEST(GpsTrack_ClearWithoutObserversStartsFreshRecording)
{
  auto const filePath = GetFilePath();
  FileWriter::DeleteFileX(filePath);
  SCOPE_GUARD(deleteFile, [&] { FileWriter::DeleteFileX(filePath); });
  auto points = MakePoints();
  {
    GpsTrackStorage storage(filePath);
    storage.Append(points);
  }
  for (auto & point : points)
  {
    point.m_timestamp -= 500;
    point.m_latitude += 1;
  }
  {
    GpsTrack track(filePath, std::make_unique<GpsTrackFilter>());
    track.Clear();
    TEST_EQUAL(track.GetTrackStatistics().m_length, 0, ());
    track.AddPoints(points);
    CheckPoints(track, points);
    track.Clear();
    TEST(track.IsEmpty(), ());
    TEST_EQUAL(track.Finalize(), 0, ());
    track.AddPoints(points);
    CheckPoints(track, points);
  }
  GpsTrack restored(filePath, std::make_unique<GpsTrackFilter>());
  CheckPoints(restored, points);
}

UNIT_TEST(GpsTrack_RendererAttachmentDoesNotOwnRecording)
{
  auto const filePath = GetFilePath();
  FileWriter::DeleteFileX(filePath);
  SCOPE_GUARD(deleteFile, [&] { FileWriter::DeleteFileX(filePath); });
  auto const points = MakePoints();
  std::promise<size_t> rendered;
  auto result = rendered.get_future();
  GpsTrack track(filePath);
  track.AddPoints({points[0], points[1]});
  TEST(!track.IsEmpty(), ());
  track.SetCallback([&](auto && toAdd, auto const &, auto const &) { rendered.set_value(toAdd.size()); });
  TEST(result.wait_for(std::chrono::seconds(5)) == std::future_status::ready, ());
  TEST_EQUAL(result.get(), 2, ());
  track.SetCallback(nullptr);
  track.AddPoints({points[2], points[3], points[4]});
  CheckPoints(track, points);
}
}  // namespace gps_track_lifecycle_test
