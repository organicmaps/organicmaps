#include "testing/testing.hpp"

#include "routing/speed_camera_manager.hpp"
#include "routing/speed_camera_ser_des.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/math.hpp"

#include <memory>
#include <vector>

using namespace platform::tests_support;
using namespace platform;
using namespace routing;

namespace
{
static auto const kTestDir = "ser_des_test_camera";
static auto const kTestFileForCamera = "ser_des_test_camera.bin";
static auto const kErrorMessageOneWay = "Serialize method works only with cameras with one way";

UNIT_TEST(SegmentCoord_LessOperator)
{
  {
    SegmentCoord a(10, 5);
    SegmentCoord b(11, 0);

    TEST_LESS(a, b, ());
  }

  {
    SegmentCoord a(10, 0);
    SegmentCoord b(11, 5);

    TEST_LESS(a, b, ());
  }

  {
    SegmentCoord a(5, 0);
    SegmentCoord b(11, 5);

    TEST_LESS(a, b, ());
  }

  {
    SegmentCoord a(5, 0);
    SegmentCoord b(0, 5);

    TEST_LESS(b, a, ());
  }

  {
    SegmentCoord a(5, 0);
    SegmentCoord b(5, 5);

    TEST_LESS(a, b, ());
  }

  {
    SegmentCoord a(5, 6);
    SegmentCoord b(5, 5);

    TEST_LESS(b, a, ());
  }

  {
    SegmentCoord a(4, 6);
    SegmentCoord b(5, 6);

    TEST_LESS(a, b, ());
  }
}

// Test for serialize/deserialize of speed cameras.

bool TestSerDesSpeedCamera(std::vector<SpeedCameraMetadata> const & speedCamerasMetadata)
{
  if (speedCamerasMetadata.empty())
    return true;

  CHECK_EQUAL(speedCamerasMetadata[0].m_ways.size(), 1, (kErrorMessageOneWay));
  for (size_t i = 1; i < speedCamerasMetadata.size(); ++i)
  {
    CHECK_EQUAL(speedCamerasMetadata[i].m_ways.size(), 1, (kErrorMessageOneWay));
    CHECK_LESS_OR_EQUAL(speedCamerasMetadata[i - 1].m_ways.back().m_featureId,
                        speedCamerasMetadata[i].m_ways.back().m_featureId,
                        ("Ways of cameras should be sorted by featureId for delta coding"));
  }

  ScopedDir scopedDir(kTestDir);
  auto const testFile = base::JoinPath(kTestDir, kTestFileForCamera);
  ScopedFile scopedFile(testFile, "");
  auto const & writableDir = GetPlatform().WritableDir();
  auto const & filePath = base::JoinPath(writableDir, testFile);

  {
    FileWriter writer(filePath);
    uint32_t prevFeatureId = 0;
    for (auto const & metadata : speedCamerasMetadata)
      SerializeSpeedCamera(writer, metadata, prevFeatureId);
  }

  {
    FileReader reader(filePath);
    ReaderSource<FileReader> src(reader);
    uint32_t prevFeatureId = 0;
    for (auto const & metadata : speedCamerasMetadata)
    {
      auto const & way = metadata.m_ways.back();
      auto const res = DeserializeSpeedCamera(src, prevFeatureId);
      TEST_EQUAL(res.first, SegmentCoord(way.m_featureId, way.m_segmentId), ());
      TEST(AlmostEqualAbs(res.second.m_coef, way.m_coef, 1e-5), ());
      TEST_EQUAL(res.second.m_maxSpeedKmPH, metadata.m_maxSpeedKmPH, ());
    }
  }

  return true;
}

UNIT_TEST(SpeedCamera_SerDes_1)
{
  std::vector<routing::SpeedCameraMwmPosition> ways = {{1 /* featureId */, 1 /* segmentId */, 0 /* coef */}};
  SpeedCameraMetadata metadata({10, 10} /* m_center */, 60 /* m_maxSpeedKmPH */, std::move(ways));

  TEST(TestSerDesSpeedCamera({metadata}), ());
}

UNIT_TEST(SpeedCamera_SerDes_2)
{
  std::vector<SpeedCameraMetadata> speedCamerasMetadata;
  {
    std::vector<routing::SpeedCameraMwmPosition> ways = {{1 /* featureId */, 1 /* segmentId */, 0 /* coef */}};
    SpeedCameraMetadata metadata({10, 10} /* m_center */, 60 /* m_maxSpeedKmPH */, std::move(ways));
    speedCamerasMetadata.emplace_back(metadata);
  }
  {
    std::vector<routing::SpeedCameraMwmPosition> ways = {{2 /* featureId */, 1 /* segmentId */, 0.5 /* coef */}};
    SpeedCameraMetadata metadata({15, 10} /* m_center */, 90 /* m_maxSpeedKmPH */, std::move(ways));
    speedCamerasMetadata.emplace_back(metadata);
  }

  TEST(TestSerDesSpeedCamera(speedCamerasMetadata), ());
}

UNIT_TEST(SpeedCamera_SerDes_3)
{
  std::vector<SpeedCameraMetadata> speedCamerasMetadata;
  {
    std::vector<routing::SpeedCameraMwmPosition> ways = {{1 /* featureId */, 1 /* segmentId */, 0 /* coef */}};
    SpeedCameraMetadata metadata({10, 10} /* m_center */, 60 /* m_maxSpeedKmPH */, std::move(ways));
    speedCamerasMetadata.emplace_back(metadata);
  }
  {
    std::vector<routing::SpeedCameraMwmPosition> ways = {{1 /* featureId */, 1 /* segmentId */, 0.5 /* coef */}};
    SpeedCameraMetadata metadata({10, 10} /* m_center */, 90 /* m_maxSpeedKmPH */, std::move(ways));
    speedCamerasMetadata.emplace_back(metadata);
  }
  {
    std::vector<routing::SpeedCameraMwmPosition> ways = {{10 /* featureId */, 1 /* segmentId */, 1 /* coef */}};
    SpeedCameraMetadata metadata({20, 20} /* m_center */, 40 /* m_maxSpeedKmPH */, std::move(ways));
    speedCamerasMetadata.emplace_back(metadata);
  }

  TEST(TestSerDesSpeedCamera(speedCamerasMetadata), ());
}

std::shared_ptr<Route> MakeCameraDistanceRoute(std::vector<m2::PointD> const & points)
{
  auto route = std::make_shared<Route>();
  route->SetGeometry(points.begin(), points.end());
  std::vector<RouteSegment> segments;
  double lengthMeters = 0;
  double lengthMercator = 0;
  for (uint32_t i = 0; i + 1 < points.size(); ++i)
  {
    auto const segment = i == 0 || i + 2 == points.size() ? Segment{} : Segment(0, 5, i, true);
    segments.emplace_back(segment, turns::TurnItem{}, geometry::PointWithAltitude(points[i + 1], 0),
                          RouteSegment::RoadNameInfo{});
    lengthMeters += mercator::DistanceOnEarth(points[i], points[i + 1]);
    lengthMercator += points[i].Length(points[i + 1]);
    segments.back().SetDistancesAndTime(lengthMeters, lengthMercator, i + 1);
  }
  route->SetRouteSegments(std::move(segments));
  route->SetSubroutes(
      std::vector<Route::SubrouteAttrs>{{geometry::PointWithAltitude(points.front(), 0),
                                         geometry::PointWithAltitude(points.back(), 0), 0, points.size() - 1}});
  return route;
}

UNIT_TEST(SpeedCamera_DistanceStartsAtTheSegmentBeginning)
{
  turns::sound::NotificationManager notifications;
  SpeedCameraManager manager(notifications);
  auto route = MakeCameraDistanceRoute({{0, 0}, {0, 0}, {0.001, 0}, {0.001, 0}});
  route->GetRouteSegments()[1].SetSpeedCameraInfo({{0.25, 50}});
  manager.SetRoute(route);
  location::GpsInfo gps;
  gps.m_speed = 0;
  manager.OnLocationPositionChanged(gps);
  auto const & camera = manager.GetClosestCamForTests();
  TEST(camera.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(camera.m_distFromBeginMeters, mercator::DistanceOnEarth({0, 0}, {0.00025, 0}), 1e-8, ());
}

UNIT_TEST(SpeedCamera_LongSegmentDoesNotHideANearbyCamera)
{
  turns::sound::NotificationManager notifications;
  SpeedCameraManager manager(notifications);
  auto route = MakeCameraDistanceRoute({{0, 0}, {0, 0}, {0.03, 0}, {0.031, 0}, {0.031, 0}});
  route->GetRouteSegments()[1].SetSpeedCameraInfo({{0.01, 50}});
  route->GetRouteSegments()[2].SetSpeedCameraInfo({{0.5, 30}});
  TEST_GREATER(route->GetSegLenMeters(1), 2000, ());
  manager.SetRoute(route);
  location::GpsInfo gps;
  gps.m_speed = 0;
  manager.OnLocationPositionChanged(gps);
  auto const & camera = manager.GetClosestCamForTests();
  TEST(camera.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(camera.m_distFromBeginMeters, mercator::DistanceOnEarth({0, 0}, {0.0003, 0}), 1e-8, ());

  // The next segment enters the lookahead range after we move along the long segment.
  gps.m_longitude = 0.029;
  TEST(route->MoveIterator(gps), ());
  manager.OnLocationPositionChanged(gps);
  TEST(camera.IsValid(), ());
  TEST_EQUAL(camera.m_maxSpeedKmH, 30, ());
  double const nextCameraDistance = mercator::DistanceOnEarth({0, 0}, {0.0305, 0});
  TEST_ALMOST_EQUAL_ABS(camera.m_distFromBeginMeters, nextCameraDistance, 1e-8, ());

  // The exhausted scan remains valid on the next update.
  manager.OnLocationPositionChanged(gps);
  TEST_ALMOST_EQUAL_ABS(camera.m_distFromBeginMeters, nextCameraDistance, 1e-8, ());
}
}  // namespace
