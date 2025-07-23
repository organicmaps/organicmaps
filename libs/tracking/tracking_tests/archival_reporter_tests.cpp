#include "testing/testing.hpp"

#include "tracking/archival_file.hpp"
#include "tracking/archive.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/timer.hpp"

#include <chrono>
#include <random>

using namespace tracking;

namespace
{
constexpr size_t kItemsForDump = 2 * 60 * 60;  // 2 hours of travelling
constexpr double kAccuracyEps = 1e-4;

location::GpsInfo GetStartingPoint()
{
  location::GpsInfo start;
  start.m_timestamp = 1573227904;
  start.m_horizontalAccuracy = 5;
  start.m_latitude = 11.67281;
  start.m_longitude = 5.22804;
  return start;
}

void UpdateSpeedGroup(traffic::SpeedGroup & sg)
{
  static std::random_device randomDevice;
  static std::mt19937 gen(randomDevice());
  static std::uniform_int_distribution<> dis(0, base::Underlying(traffic::SpeedGroup::Count));
  sg = static_cast<traffic::SpeedGroup>(dis(gen));
}

void UpdateLocation(location::GpsInfo & loc)
{
  static std::random_device randomDevice;
  static std::mt19937 gen(randomDevice());
  static std::uniform_int_distribution<> dis(0, 50);
  loc.m_latitude += dis(gen) * 0.0001;
  loc.m_longitude += dis(gen) * 0.0001;
  CHECK_GREATER_OR_EQUAL(loc.m_latitude, ms::LatLon::kMinLat, ());
  CHECK_LESS_OR_EQUAL(loc.m_latitude, ms::LatLon::kMaxLat, ());
  CHECK_GREATER_OR_EQUAL(loc.m_longitude, ms::LatLon::kMinLon, ());
  CHECK_LESS_OR_EQUAL(loc.m_longitude, ms::LatLon::kMaxLon, ());

  double constexpr kMinIntervalBetweenLocationsS = 3.0;
  loc.m_timestamp += kMinIntervalBetweenLocationsS + dis(gen);
}

UNIT_TEST(PacketCar_OperationsConsistency)
{
  BasicArchive<PacketCar> archive(kItemsForDump, 1.0 /* m_minDelaySeconds */);
  location::GpsInfo point = GetStartingPoint();
  traffic::SpeedGroup sg = traffic::SpeedGroup::G0;

  base::HighResTimer timer;

  for (size_t i = 0; i < kItemsForDump; ++i)
  {
    archive.Add(point, sg);
    UpdateLocation(point);
    UpdateSpeedGroup(sg);
  }

  auto const track = archive.Extract();

  LOG(LINFO, ("Duration of dumping", timer.ElapsedMilliseconds(), "ms"));

  timer.Reset();
  std::string const fileName = "archival_reporter_car.track";
  {
    FileWriter writer(fileName);
    CHECK(archive.Write(writer), ());
  }

  LOG(LINFO, ("Duration of serializing", timer.ElapsedMilliseconds(), "ms"));

  uint64_t sizeBytes;
  CHECK(GetPlatform().GetFileSizeByFullPath(fileName, sizeBytes), ());
  LOG(LINFO, ("File size", sizeBytes, "bytes"));

  FileReader reader(fileName);
  ReaderSource<FileReader> src(reader);

  CHECK(archive.Read(src), ());
  CHECK_EQUAL(kItemsForDump, archive.Size(), ());
  auto const newTrack = archive.Extract();
  for (size_t i = 0; i < kItemsForDump; ++i)
  {
    TEST_ALMOST_EQUAL_ABS(track[i].m_lat, newTrack[i].m_lat, kAccuracyEps, ("failed index", i));
    TEST_ALMOST_EQUAL_ABS(track[i].m_lon, newTrack[i].m_lon, kAccuracyEps, ("failed index", i));
    CHECK_EQUAL(track[i].m_timestamp, newTrack[i].m_timestamp, ("failed index", i));
    CHECK_EQUAL(track[i].m_speedGroup, newTrack[i].m_speedGroup, ("failed index", i));
  }
}

UNIT_TEST(PacketPedestrianBicycle_OperationsConsistency)
{
  BasicArchive<Packet> archive(kItemsForDump, 3.0 /* m_minDelaySeconds */);
  location::GpsInfo point = GetStartingPoint();

  for (size_t i = 0; i < kItemsForDump; ++i)
  {
    archive.Add(point);
    UpdateLocation(point);
  }
  auto const track = archive.Extract();

  std::string const fileName = "archival_reporter_pedestrian_bicycle.track";
  {
    FileWriter writer(fileName);
    CHECK(archive.Write(writer), (fileName));
  }
  uint64_t sizeBytes;
  CHECK(GetPlatform().GetFileSizeByFullPath(fileName, sizeBytes), (fileName, sizeBytes));
  LOG(LINFO, ("File size", sizeBytes, "bytes"));

  FileReader reader(fileName);
  ReaderSource<FileReader> src(reader);

  CHECK(archive.Read(src), ());
  CHECK_EQUAL(kItemsForDump, archive.Size(), ());
  auto const newTrack = archive.Extract();
  for (size_t i = 0; i < kItemsForDump; ++i)
  {
    TEST_ALMOST_EQUAL_ABS(track[i].m_lat, newTrack[i].m_lat, kAccuracyEps, ("failed index", i));
    TEST_ALMOST_EQUAL_ABS(track[i].m_lon, newTrack[i].m_lon, kAccuracyEps, ("failed index", i));
    CHECK_EQUAL(track[i].m_timestamp, newTrack[i].m_timestamp, ("failed index", i));
  }
}

UNIT_TEST(ArchiveName_Create)
{
  routing::RouterType const routerType = routing::RouterType::Pedestrian;
  uint32_t const version = 1;
  std::chrono::seconds const timestamp(1573635326);
  std::string const filename = tracking::archival_file::GetArchiveFilename(version, timestamp, routerType);

  CHECK_EQUAL(filename, "1_1573635326_1.track", ());
}

UNIT_TEST(ArchiveName_Extract)
{
  std::string const filename = "1_1573635326_2.track";
  auto const meta = tracking::archival_file::ParseArchiveFilename(filename);

  CHECK_EQUAL(meta.m_protocolVersion, 1, ());
  CHECK_EQUAL(meta.m_timestamp, 1573635326, ());
  CHECK_EQUAL(meta.m_trackType, routing::RouterType::Bicycle, ());
}
}  // namespace
