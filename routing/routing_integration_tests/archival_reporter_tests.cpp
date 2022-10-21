#include "testing/testing.hpp"

#include "tracking/archival_manager.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "map/framework.hpp"
#include "map/routing_manager.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"

#include "defines.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <thread>

/// @obsolete https://github.com/organicmaps/organicmaps/commit/04bc294c851bdfe3189d04391f7c3a7d6e601835

/*
namespace
{
void UpdateLocationForArchiving(location::GpsInfo & point) { point.m_timestamp += 3; }

size_t GetFilesCount(std::string const & path,
                     std::string const & extension = ARCHIVE_TRACKS_FILE_EXTENSION)
{
  Platform::FilesList files;
  Platform::GetFilesByExt(path, extension, files);
  return files.size();
}

void FillArchive(RoutingManager & manager, size_t count)
{
  location::GpsInfo point;
  point.m_horizontalAccuracy = 10.0;

  for (size_t i = 0; i < count; ++i)
  {
    UpdateLocationForArchiving(point);
    manager.OnLocationUpdate(point);
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
}

void CreateEmptyFile(std::string const & path, std::string const & fileName)
{
  FileWriter fw(base::JoinPath(path, fileName));
}

void TestFilesExistence(size_t newestIndex, size_t fileCount, std::string const & fileExtension,
                        std::string const & dir)
{
  CHECK_GREATER(newestIndex, fileCount, ());
  for (size_t i = newestIndex; i > newestIndex - fileCount; --i)
  {
    TEST(Platform::IsFileExistsByFullPath(base::JoinPath(dir, std::to_string(i) + fileExtension)),
         ());
  }
}

TRouteResult GetRouteResult()
{
  return integration::CalculateRoute(integration::GetVehicleComponents(routing::VehicleType::Car),
                                     mercator::FromLatLon(55.7607268, 37.5801099), m2::PointD::Zero(),
                                     mercator::FromLatLon(55.75718, 37.63156));
}

size_t GetInitialTimestamp()
{
  auto const now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

class TestArchivalReporter
{
public:
  TestArchivalReporter()
    : m_framework(m_frameworkParams)
    , m_manager(m_framework.GetRoutingManager())
    , m_session(m_manager.RoutingSession())
    , m_routeResult(GetRouteResult())
    , m_route(*m_routeResult.first)
    , m_tracksDir(tracking::GetTracksDirectory())
  {
    RouterResultCode const result = m_routeResult.second;
    TEST_EQUAL(result, RouterResultCode::NoError, ());

    m_manager.SetRouter(routing::RouterType::Vehicle);
    m_session.SetState(routing::SessionState::OnRoute);
    m_session.EnableFollowMode();

    m_session.SetRoutingSettings(routing::GetRoutingSettings(routing::VehicleType::Car));
    m_session.AssignRouteForTesting(std::make_shared<Route>(m_route), m_routeResult.second);
  }

  ~TestArchivalReporter() { Platform::RmDirRecursively(m_tracksDir); }

protected:
  FrameworkParams m_frameworkParams;
  Framework m_framework;
  RoutingManager & m_manager;
  routing::RoutingSession & m_session;
  TRouteResult m_routeResult;
  Route & m_route;
  std::string m_tracksDir;
};
} // namespace

// Ordinary ArchivalReporter pipeline: periodically dump files.
UNIT_CLASS_TEST(TestArchivalReporter, StraightPipeline)
{
  TEST_EQUAL(GetFilesCount(m_tracksDir), 0, ());
  for (size_t iter = 1; iter < 4; ++iter)
  {
    FillArchive(m_manager, tracking::kItemsForDump);
    TEST_EQUAL(GetFilesCount(m_tracksDir), iter, ());
  }
}

// Startup of ArchivalReporter: if there are too many files they need to be removed.
UNIT_TEST(TestArchivalReporter_DeleteOldData)
{
  tracking::ArchivingSettings const settings;
  std::string const tracksDir = tracking::GetTracksDirectory();
  CHECK(Platform::MkDirChecked(tracksDir), ());
  size_t const maxFilesCount = std::max(settings.m_maxFilesToSave, settings.m_maxArchivesToSave);
  size_t const tsStart = GetInitialTimestamp();
  size_t newestFileIndex = maxFilesCount * 2;

  // Create files before the AchivalReporter initialization.
  for (size_t i = 0, ts = tsStart; i <= newestFileIndex; ++i, ++ts)
  {
    auto const name = std::to_string(ts);
    CreateEmptyFile(tracksDir, name + ARCHIVE_TRACKS_FILE_EXTENSION);
    CreateEmptyFile(tracksDir, name + ARCHIVE_TRACKS_ZIPPED_FILE_EXTENSION);
    TEST_EQUAL(GetFilesCount(tracksDir, ARCHIVE_TRACKS_FILE_EXTENSION), i + 1, ());
    TEST_EQUAL(GetFilesCount(tracksDir, ARCHIVE_TRACKS_ZIPPED_FILE_EXTENSION), i + 1, ());
  }

  // Create the ArchivalReporter instance. It will remove the oldest files on startup.
  TestArchivalReporter testReporter;
  std::this_thread::sleep_for(std::chrono::seconds(2));
  // Check that the files count equals to the maximum count.
  TEST_EQUAL(GetFilesCount(tracksDir, ARCHIVE_TRACKS_FILE_EXTENSION), settings.m_maxFilesToSave,
             ());
  TEST_EQUAL(GetFilesCount(tracksDir, ARCHIVE_TRACKS_ZIPPED_FILE_EXTENSION),
             settings.m_maxArchivesToSave, ());
  // Check that the oldest files are removed and the newest ones are not.
  newestFileIndex += tsStart;
  TestFilesExistence(newestFileIndex, settings.m_maxFilesToSave, ARCHIVE_TRACKS_FILE_EXTENSION,
                     tracksDir);
  TestFilesExistence(newestFileIndex, settings.m_maxArchivesToSave,
                     ARCHIVE_TRACKS_ZIPPED_FILE_EXTENSION, tracksDir);
}

// ArchivalReporter pipeline with no dumping.
// Checks behaviour if there is no free space on device.
UNIT_CLASS_TEST(TestArchivalReporter, FreeSpaceOnDisk)
{
  tracking::ArchivingSettings settings;
  settings.m_minFreeSpaceOnDiskBytes = std::numeric_limits<size_t>::max();
  m_manager.ConfigureArchivalReporter(settings);

  TEST_EQUAL(GetFilesCount(m_tracksDir), 0, ());
  for (size_t iter = 1; iter < 4; ++iter)
  {
    FillArchive(m_manager, tracking::kItemsForDump);
    TEST_EQUAL(GetFilesCount(m_tracksDir), 0, ());
  }
}
*/
