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

UNIT_TEST(GpsTrackFile_WriteReadWithoutTrunc)
{
  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  string const filePath = GetGpsTrackFilePath();
  MY_SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  size_t const fileMaxItemCount = 100000;

  vector<location::GpsTrackInfo> points;
  points.reserve(fileMaxItemCount);
  for (size_t i = 0; i < fileMaxItemCount; ++i)
    points.emplace_back(Make(timestamp + i, ms::LatLon(-90 + i, -180 + i), 60 + i));

  // Create file, write data and check written data
  {
    GpsTrackFile file;
    TEST(file.Create(filePath, fileMaxItemCount), ());
    TEST(file.IsOpen(), ());

    file.Append(points);

    size_t i = 0;
    file.ForEach([&](location::GpsTrackInfo const & point)->bool
    {
      TEST_EQUAL(point.m_latitude, points[i].m_latitude, ());
      TEST_EQUAL(point.m_longitude, points[i].m_longitude, ());
      TEST_EQUAL(point.m_timestamp, points[i].m_timestamp, ());
      TEST_EQUAL(point.m_speed, points[i].m_speed, ());
      ++i;
      return true;
    });
    TEST_EQUAL(i, fileMaxItemCount, ());

    file.Close();
    TEST(!file.IsOpen(), ());
  }

  // Open file and check previously written data
  {
    GpsTrackFile file;
    TEST(file.Open(filePath, fileMaxItemCount), ());
    TEST(file.IsOpen(), ());

    size_t i = 0;
    file.ForEach([&](location::GpsTrackInfo const & point)->bool
    {
      TEST_EQUAL(point.m_latitude, points[i].m_latitude, ());
      TEST_EQUAL(point.m_longitude, points[i].m_longitude, ());
      TEST_EQUAL(point.m_timestamp, points[i].m_timestamp, ());
      TEST_EQUAL(point.m_speed, points[i].m_speed, ());
      ++i;
      return true;
    });
    TEST_EQUAL(i, fileMaxItemCount, ());

    // Clear data
    file.Clear();

    file.Close();
    TEST(!file.IsOpen(), ());
  }

  // Open file and check there is no data
  {
    GpsTrackFile file;
    TEST(file.Open(filePath, fileMaxItemCount), ());
    TEST(file.IsOpen(), ());

    size_t i = 0;
    file.ForEach([&](location::GpsTrackInfo const & point)->bool{ ++i; return true; });
    TEST_EQUAL(i, 0, ());

    file.Close();
    TEST(!file.IsOpen(), ());
  }
}

UNIT_TEST(GpsTrackFile_WriteReadWithTrunc)
{
  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  string const filePath = GetGpsTrackFilePath();
  MY_SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  size_t const fileMaxItemCount = 100000;

  vector<location::GpsTrackInfo> points1, points2;
  points1.reserve(fileMaxItemCount);
  points2.reserve(fileMaxItemCount);
  for (size_t i = 0; i < fileMaxItemCount; ++i)
  {
    points1.emplace_back(Make(timestamp + i, ms::LatLon(-90 + i, -180 + i), 60 + i));
    points2.emplace_back(Make(timestamp + i + fileMaxItemCount, ms::LatLon(-45 + i, -30 + i), 15 + i));
  }

  vector<location::GpsTrackInfo> points3;
  points3.reserve(fileMaxItemCount/2);
  for (size_t i = 0; i < fileMaxItemCount/2; ++i)
    points3.emplace_back(Make(timestamp + i, ms::LatLon(-30 + i, -60 + i), 90 + i));

  // Create file and write blob 1
  {
    GpsTrackFile file;
    TEST(file.Create(filePath, fileMaxItemCount), ());
    TEST(file.IsOpen(), ());

    file.Append(points1);

    file.Close();
    TEST(!file.IsOpen(), ());
  }

  // Create file and write blob 2
  {
    GpsTrackFile file;
    TEST(file.Open(filePath, fileMaxItemCount), ());
    TEST(file.IsOpen(), ());

    file.Append(points2);

    file.Close();
    TEST(!file.IsOpen(), ());
  }

  // Create file and write blob 3
  {
    GpsTrackFile file;
    TEST(file.Open(filePath, fileMaxItemCount), ());
    TEST(file.IsOpen(), ());

    file.Append(points3); // trunc happens here

    file.Close();
    TEST(!file.IsOpen(), ());
  }

  // Check file must contain second half of points2 and points3
  {
    GpsTrackFile file;
    TEST(file.Open(filePath, fileMaxItemCount), ());
    TEST(file.IsOpen(), ());

    size_t i = 0;
    file.ForEach([&](location::GpsTrackInfo const & point)->bool
    {
      if (i < fileMaxItemCount/2)
      {
        TEST_EQUAL(point.m_latitude, points2[fileMaxItemCount/2 + i].m_latitude, ());
        TEST_EQUAL(point.m_longitude, points2[fileMaxItemCount/2 + i].m_longitude, ());
        TEST_EQUAL(point.m_timestamp, points2[fileMaxItemCount/2 + i].m_timestamp, ());
        TEST_EQUAL(point.m_speed, points2[fileMaxItemCount/2 + i].m_speed, ());
      }
      else
      {
        TEST_EQUAL(point.m_latitude, points3[i - fileMaxItemCount/2].m_latitude, ());
        TEST_EQUAL(point.m_longitude, points3[i - fileMaxItemCount/2].m_longitude, ());
        TEST_EQUAL(point.m_timestamp, points3[i - fileMaxItemCount/2].m_timestamp, ());
        TEST_EQUAL(point.m_speed, points3[i - fileMaxItemCount/2].m_speed, ());
      }
      ++i;
      return true;
    });
    TEST_EQUAL(i, fileMaxItemCount, ());

    // Clear data
    file.Clear();

    file.Close();
    TEST(!file.IsOpen(), ());
  }

  // Check no data in file
  {
    GpsTrackFile file;
    TEST(file.Open(filePath, fileMaxItemCount), ());
    TEST(file.IsOpen(), ());

    size_t i = 0;
    file.ForEach([&](location::GpsTrackInfo const & point)->bool{ ++i; return true; });
    TEST_EQUAL(i, 0, ());

    file.Close();
    TEST(!file.IsOpen(), ());
  }
}
