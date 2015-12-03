#include "testing/testing.hpp"

#include "map/gps_track_file.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"

#include "geometry/latlon.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/chrono.hpp"

namespace
{

location::GpsTrackInfo Make(double timestamp, ms::LatLon const & ll, double speed)
{
  location::GpsTrackInfo info;
  info.m_timestamp = timestamp;
  info.m_speed = speed;
  info.m_latitude = ll.lat;
  info.m_longitude = ll.lon;
  return info;
}

inline string GetGpsTrackFilePath()
{
  return my::JoinFoldersToPath(GetPlatform().WritableDir(), "gpstrack.bin");
}

} // namespace

UNIT_TEST(GpsTrackFile_SimpleWriteRead)
{
  string const filePath = GetGpsTrackFilePath();
  MY_SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  size_t const fileMaxItemCount = 100000;

  // Write GPS tracks.
  // (write only half of max items to do not do recycling)
  {
    GpsTrackFile file;
    file.Create(filePath, fileMaxItemCount);

    TEST(file.IsOpen(), ());

    TEST_EQUAL(fileMaxItemCount, file.GetMaxCount(), ());

    TEST_EQUAL(0, file.GetCount(), ());

    for (size_t i = 0; i < fileMaxItemCount / 2; ++i)
    {
      size_t evictedId;
      size_t addedId = file.Append(Make(timestamp + i, ms::LatLon(i + 1000, i + 2000), i + 3000), evictedId);
      TEST_EQUAL(i, addedId, ());
      TEST_EQUAL(GpsTrackFile::kInvalidId, evictedId, ());
    }

    TEST_EQUAL(fileMaxItemCount / 2, file.GetCount(), ());

    file.Close();

    TEST(!file.IsOpen(), ());
  }

  // Read GPS tracks.
  {
    GpsTrackFile file;
    file.Open(filePath, fileMaxItemCount);

    TEST(file.IsOpen(), ());
    TEST(!file.IsEmpty(), ());

    TEST_EQUAL(fileMaxItemCount, file.GetMaxCount(), ());

    TEST_EQUAL(fileMaxItemCount/2, file.GetCount(), ());

    size_t i = 0;
    file.ForEach([&i,timestamp](location::GpsTrackInfo const & info, size_t id)->bool
    {
      TEST_EQUAL(id, i, ());
      TEST_EQUAL(info.m_timestamp, timestamp + i, ());
      TEST_EQUAL(info.m_latitude, i + 1000, ());
      TEST_EQUAL(info.m_longitude, i + 2000, ());
      TEST_EQUAL(info.m_speed, i + 3000, ());
      ++i;
      return true;
    });

    TEST_EQUAL(i, fileMaxItemCount / 2, ());

    auto res = file.Clear();
    TEST_EQUAL(res.first, 0, ());
    TEST_EQUAL(res.second, fileMaxItemCount / 2 - 1, ());

    TEST(file.IsEmpty(), ());

    file.Close();

    TEST(!file.IsOpen(), ());
  }
}

UNIT_TEST(GpsTrackFile_WriteReadWithEvicting)
{
  string const filePath = GetGpsTrackFilePath();
  MY_SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  size_t const fileMaxItemCount = 100000;

  // Write GPS tracks.
  // 2 x fileMaxItemCount more items are written in cyclic file, first half will be evicted
  {
    GpsTrackFile file;
    file.Create(filePath, fileMaxItemCount);

    TEST(file.IsOpen(), ());

    TEST_EQUAL(fileMaxItemCount, file.GetMaxCount(), ());

    TEST_EQUAL(0, file.GetCount(), ());

    for (size_t i = 0; i < 2 * fileMaxItemCount; ++i)
    {
      size_t evictedId;
      size_t addedId = file.Append(Make(timestamp + i, ms::LatLon(i + 1000, i + 2000), i + 3000), evictedId);
      TEST_EQUAL(i, addedId, ());
      if (i >= fileMaxItemCount)
      {
        TEST_EQUAL(i - fileMaxItemCount, evictedId, ());
      }
      else
      {
        TEST_EQUAL(GpsTrackFile::kInvalidId, evictedId, ());
      }
    }

    TEST_EQUAL(fileMaxItemCount, file.GetCount(), ());

    file.Close();

    TEST(!file.IsOpen(), ());
  }

  // Read GPS tracks
  // Only last fileMaxItemCount must be in cyclic buffer
  {
    GpsTrackFile file;
    file.Open(filePath, fileMaxItemCount);

    TEST(file.IsOpen(), ());
    TEST(!file.IsEmpty(), ());

    TEST_EQUAL(fileMaxItemCount, file.GetMaxCount(), ());
    TEST_EQUAL(fileMaxItemCount, file.GetCount(), ());

    size_t i = 0;
    file.ForEach([&i,timestamp](location::GpsTrackInfo const & info, size_t id)->bool
    {
      TEST_EQUAL(id, i + fileMaxItemCount, ());
      TEST_EQUAL(info.m_timestamp, timestamp + i + fileMaxItemCount, ());
      TEST_EQUAL(info.m_latitude, i + 1000 + fileMaxItemCount, ());
      TEST_EQUAL(info.m_longitude, i + 2000 + fileMaxItemCount, ());
      TEST_EQUAL(info.m_speed, i + 3000 + fileMaxItemCount, ());
      ++i;
      return true;
    });

    TEST_EQUAL(i, fileMaxItemCount, ());

    file.Close();

    TEST(!file.IsOpen(), ());
  }
}

UNIT_TEST(GpsTrackFile_DropInTail)
{
  string const filePath = GetGpsTrackFilePath();
  MY_SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  GpsTrackFile file;
  file.Create(filePath, 100);

  TEST(file.IsOpen(), ());

  TEST_EQUAL(100, file.GetMaxCount(), ());

  for (size_t i = 0; i < 50; ++i)
  {
    size_t evictedId;
    size_t addedId = file.Append(Make(timestamp + i, ms::LatLon(i + 1000, i + 2000), i + 3000), evictedId);
    TEST_EQUAL(i, addedId, ());
    TEST_EQUAL(GpsTrackFile::kInvalidId, evictedId, ());
  }

  TEST_EQUAL(50, file.GetCount(), ());

  auto res = file.DropEarlierThan(timestamp + 4.5); // drop points 0,1,2,3,4
  TEST_EQUAL(res.first, 0, ());
  TEST_EQUAL(res.second, 4, ());

  TEST_EQUAL(45, file.GetCount(), ());

  size_t i = 5; // new first
  file.ForEach([&i,timestamp](location::GpsTrackInfo const & info, size_t id)->bool
  {
    TEST_EQUAL(info.m_timestamp, timestamp + i, ());
    TEST_EQUAL(info.m_latitude, i + 1000, ());
    TEST_EQUAL(info.m_longitude, i + 2000, ());
    TEST_EQUAL(info.m_speed, i + 3000, ());
    ++i;
    return true;
  });

  res = file.Clear();
  TEST_EQUAL(res.first, 5, ());
  TEST_EQUAL(res.second, 49, ());

  TEST(file.IsEmpty(), ())

  file.Close();

  TEST(!file.IsOpen(), ());
}

UNIT_TEST(GpsTrackFile_DropInMiddle)
{
  string const filePath = GetGpsTrackFilePath();
  MY_SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  GpsTrackFile file;
  file.Create(filePath, 100);

  TEST(file.IsOpen(), ());

  TEST_EQUAL(100, file.GetMaxCount(), ());

  for (size_t i = 0; i < 50; ++i)
  {
    size_t evictedId;
    size_t addedId = file.Append(Make(timestamp + i, ms::LatLon(i + 1000,i + 2000), i + 3000), evictedId);
    TEST_EQUAL(i, addedId, ());
    TEST_EQUAL(GpsTrackFile::kInvalidId, evictedId, ());
  }

  TEST_EQUAL(50, file.GetCount(), ());

  auto res = file.DropEarlierThan(timestamp + 48.5); // drop all except last
  TEST_EQUAL(res.first, 0, ());
  TEST_EQUAL(res.second, 48, ());

  TEST_EQUAL(1, file.GetCount(), ());

  size_t i = 49; // new first
  file.ForEach([&i,timestamp](location::GpsTrackInfo const & info, size_t id)->bool
  {
    TEST_EQUAL(info.m_timestamp, timestamp + i, ());
    TEST_EQUAL(info.m_latitude, i + 1000, ());
    TEST_EQUAL(info.m_longitude, i + 2000, ());
    TEST_EQUAL(info.m_speed, i + 3000, ());
    ++i;
    return true;
  });

  file.Close();

  TEST(!file.IsOpen(), ());
}

UNIT_TEST(GpsTrackFile_DropAll)
{
  string const filePath = GetGpsTrackFilePath();
  MY_SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  GpsTrackFile file;
  file.Create(filePath, 100);

  TEST(file.IsOpen(), ());

  TEST_EQUAL(100, file.GetMaxCount(), ());

  for (size_t i = 0; i < 50; ++i)
  {
    size_t evictedId;
    size_t addedId = file.Append(Make(timestamp + i, ms::LatLon(i + 1000, i + 2000), i + 3000), evictedId);
    TEST_EQUAL(i, addedId, ());
    TEST_EQUAL(GpsTrackFile::kInvalidId, evictedId, ());
  }

  TEST_EQUAL(50, file.GetCount(), ());

  auto res = file.DropEarlierThan(timestamp + 51); // drop all
  TEST_EQUAL(res.first, 0, ());
  TEST_EQUAL(res.second, 49, ());

  TEST(file.IsEmpty(), ());

  TEST_EQUAL(0, file.GetCount(), ());

  file.Close();

  TEST(!file.IsOpen(), ());
}

UNIT_TEST(GpsTrackFile_Clear)
{
  string const filePath = GetGpsTrackFilePath();
  MY_SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  GpsTrackFile file;
  file.Create(filePath, 100);

  TEST(file.IsOpen(), ());

  TEST_EQUAL(100, file.GetMaxCount(), ());

  for (size_t i = 0; i < 50; ++i)
  {
    size_t evictedId;
    size_t addedId = file.Append(Make(timestamp + i, ms::LatLon(i + 1000, i + 2000), i + 3000), evictedId);
    TEST_EQUAL(i, addedId, ());
    TEST_EQUAL(GpsTrackFile::kInvalidId, evictedId, ());
  }

  TEST_EQUAL(50, file.GetCount(), ());

  auto res = file.Clear();
  TEST_EQUAL(res.first, 0, ());
  TEST_EQUAL(res.second, 49, ());

  TEST(file.IsEmpty(), ());

  TEST_EQUAL(0, file.GetCount(), ());

  file.Close();

  TEST(!file.IsOpen(), ());
}

UNIT_TEST(GpsTrackFile_CreateOpenClose)
{
  string const filePath = GetGpsTrackFilePath();
  MY_SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  {
    GpsTrackFile file;
    file.Create(filePath, 100);

    TEST(file.IsOpen(), ());

    TEST_EQUAL(100, file.GetMaxCount(), ());
    TEST_EQUAL(0, file.GetCount(), ());

    file.Close();

    TEST(!file.IsOpen(), ());
  }

  {
    GpsTrackFile file;
    file.Open(filePath, 100);

    TEST(file.IsOpen(), ());

    TEST(file.IsEmpty(), ());
    TEST_EQUAL(100, file.GetMaxCount(), ());
    TEST_EQUAL(0, file.GetCount(), ());

    auto res = file.Clear();
    TEST_EQUAL(res.first, GpsTrackFile::kInvalidId, ());
    TEST_EQUAL(res.second, GpsTrackFile::kInvalidId, ());

    file.Close();

    TEST(!file.IsOpen(), ());
  }
}
