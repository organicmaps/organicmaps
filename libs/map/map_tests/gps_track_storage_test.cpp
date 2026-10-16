#include "testing/testing.hpp"

#include "map/gps_track_storage.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>
#include <vector>

namespace gps_track_storage_test
{
namespace fs = std::filesystem;

namespace
{
location::GpsInfo Make(size_t i)
{
  location::GpsInfo info;
  info.m_timestamp = 1000. + i;
  info.m_latitude = 10. + i * 0.00001;
  info.m_longitude = 20. + i * 0.00001;
  info.m_altitude = 100. + i;
  info.m_speed = 5. + i;
  info.m_bearing = 90. + i;
  info.m_horizontalAccuracy = 10. + i;
  info.m_verticalAccuracy = 20. + i;
  info.m_source = location::EAndroidNative;
  return info;
}

class TestBackup
{
public:
  TestBackup() : m_directory(GetPlatform().TmpPathForFile("gps_track_storage_test_", ""))
  {
    TEST(fs::create_directory(m_directory), (m_directory.string()));
  }

  ~TestBackup()
  {
    std::error_code error;
    fs::remove_all(m_directory, error);
    if (error)
      LOG(LERROR, ("Cannot remove GPS track test directory:", m_directory.string(), error.message()));
  }

  fs::path GetFilePath() const { return m_directory / "backup"; }

private:
  fs::path const m_directory;
};

void WriteFile(fs::path const & path, std::string_view contents = {}, std::ios::openmode mode = std::ios::trunc)
{
  std::ofstream stream(path, std::ios::binary | mode);
  if (!contents.empty())
    stream.write(contents.data(), contents.size());
  stream.close();
  TEST(stream, (path.string()));
}

void CheckPoints(std::vector<location::GpsInfo> const & actual, std::vector<location::GpsInfo> const & expected)
{
  TEST_EQUAL(actual.size(), expected.size(), ());
  for (size_t i = 0; i < actual.size(); ++i)
  {
    TEST_EQUAL(actual[i].m_timestamp, expected[i].m_timestamp, (i));
    TEST_EQUAL(actual[i].m_latitude, expected[i].m_latitude, (i));
    TEST_EQUAL(actual[i].m_longitude, expected[i].m_longitude, (i));
    TEST_EQUAL(actual[i].m_altitude, expected[i].m_altitude, (i));
    TEST_EQUAL(actual[i].m_speed, expected[i].m_speed, (i));
    TEST_EQUAL(actual[i].m_bearing, expected[i].m_bearing, (i));
    TEST_EQUAL(actual[i].m_horizontalAccuracy, expected[i].m_horizontalAccuracy, (i));
    TEST_EQUAL(actual[i].m_verticalAccuracy, expected[i].m_verticalAccuracy, (i));
    TEST_EQUAL(actual[i].m_source, expected[i].m_source, (i));
  }
}
}  // namespace

UNIT_TEST(GpsTrackStorage_MissingBackup)
{
  TestBackup const backup;
  auto const filePath = backup.GetFilePath();

  GpsTrackStorage storage(filePath.string());
  TEST(!fs::exists(filePath), ());
  TEST(storage.Load().empty(), ());
  TEST(storage.Append({}), ());
  storage.Clear();
  TEST(!fs::exists(filePath), ());
}

UNIT_TEST(GpsTrackStorage_WriteRead)
{
  TestBackup const backup;
  auto const filePath = backup.GetFilePath();

  std::vector<location::GpsInfo> points;
  for (size_t i = 0; i < 1003; ++i)
    points.push_back(Make(i));

  {
    GpsTrackStorage storage(filePath.string());
    TEST(storage.Append(points), ());
    CheckPoints(storage.Load(), points);
  }

  {
    GpsTrackStorage storage(filePath.string());
    CheckPoints(storage.Load(), points);

    std::vector<location::GpsInfo> const extra = {Make(points.size()), Make(points.size() + 1)};
    TEST(storage.Append(extra), ());
    points.insert(points.end(), extra.begin(), extra.end());
    CheckPoints(storage.Load(), points);

    storage.Clear();
    TEST(!fs::exists(filePath), ());
    TEST(storage.Load().empty(), ());

    TEST(storage.Append({points.back()}), ());
    CheckPoints(storage.Load(), {points.back()});
  }
}

UNIT_TEST(GpsTrackStorage_EmptyBackup)
{
  TestBackup const backup;
  auto const filePath = backup.GetFilePath();

  std::array<char, sizeof(uint32_t)> const header = {1};
  WriteFile(filePath, {header.data(), header.size()});

  GpsTrackStorage storage(filePath.string());
  TEST(storage.Load().empty(), ());
  TEST(fs::exists(filePath), ());
  TEST(storage.Append({Make(0)}), ());
  CheckPoints(storage.Load(), {Make(0)});
}

UNIT_TEST(GpsTrackStorage_UnsupportedVersion)
{
  base::ScopedLogAbortLevelChanger const expectedError(LCRITICAL);
  TestBackup const backup;
  auto const filePath = backup.GetFilePath();

  std::array<char, sizeof(uint32_t)> const header = {};
  WriteFile(filePath, {header.data(), header.size()});

  GpsTrackStorage storage(filePath.string());
  TEST(storage.Load().empty(), ());
  TEST(!fs::exists(filePath), ());
  TEST(storage.Append({Make(0)}), ());
  CheckPoints(storage.Load(), {Make(0)});
}

UNIT_TEST(GpsTrackStorage_IncompleteHeader)
{
  base::ScopedLogAbortLevelChanger const expectedError(LCRITICAL);
  TestBackup const backup;
  auto const filePath = backup.GetFilePath();

  for (size_t size = 0; size < sizeof(uint32_t); ++size)
  {
    std::array<char, sizeof(uint32_t)> const bytes = {};
    WriteFile(filePath, {bytes.data(), size});
    GpsTrackStorage storage(filePath.string());
    TEST(storage.Load().empty(), (size));
    TEST(!fs::exists(filePath), (size));
    TEST(storage.Append({Make(0)}), (size));
    CheckPoints(storage.Load(), {Make(0)});
  }
}

UNIT_TEST(GpsTrackStorage_IncompletePoint)
{
  base::ScopedLogAbortLevelChanger const expectedError(LCRITICAL);
  TestBackup const backup;
  auto const filePath = backup.GetFilePath();

  {
    GpsTrackStorage storage(filePath.string());
    TEST(storage.Append({Make(0)}), ());
  }
  std::array<char, 17> const partialPoint = {};
  WriteFile(filePath, {partialPoint.data(), partialPoint.size()}, std::ios::app);

  GpsTrackStorage storage(filePath.string());
  CheckPoints(storage.Load(), {Make(0)});
  TEST(storage.Append({Make(1)}), ());
  CheckPoints(storage.Load(), {Make(0), Make(1)});
  TEST_EQUAL(fs::file_size(filePath), sizeof(uint32_t) + 2 * (8 * sizeof(double) + sizeof(uint8_t)), ());
}

UNIT_TEST(GpsTrackStorage_FailedAppendRecovers)
{
  base::ScopedLogAbortLevelChanger const expectedError(LCRITICAL);
  TestBackup const backup;
  auto const directoryPath = backup.GetFilePath();
  auto const filePath = directoryPath / "backup";
  WriteFile(directoryPath);

  GpsTrackStorage storage(filePath.string());
  TEST(storage.Load().empty(), ());
  TEST(!storage.Append({Make(0), Make(1)}), ());
  TEST(!storage.Append({Make(2)}), ());

  TEST(fs::remove(directoryPath), ());
  TEST(fs::create_directory(directoryPath), ());
  TEST(storage.Append({Make(3)}), ());
  CheckPoints(storage.Load(), {Make(3)});
  TEST_EQUAL(fs::file_size(filePath), sizeof(uint32_t) + 8 * sizeof(double) + sizeof(uint8_t), ());
}

UNIT_TEST(GpsTrackStorage_FailedLoadAndClearRecover)
{
  base::ScopedLogAbortLevelChanger const expectedError(LCRITICAL);
  TestBackup const backup;
  auto const filePath = backup.GetFilePath();
  auto const childPath = filePath / "child";
  TEST(fs::create_directory(filePath), ());
  WriteFile(childPath);

  GpsTrackStorage storage(filePath.string());
  TEST(storage.Load().empty(), ());
  TEST(fs::exists(childPath), ());
  TEST(!storage.Append({Make(0)}), ());
  storage.Clear();
  TEST(fs::exists(childPath), ());

  TEST(fs::remove(childPath), ());
  TEST(fs::remove(filePath), ());
  TEST(storage.Append({Make(1)}), ());
  CheckPoints(storage.Load(), {Make(1)});
}
}  // namespace gps_track_storage_test
