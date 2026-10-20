#include "testing/testing.hpp"

#include "routing/routing_helpers.hpp"
#include "routing/routing_tests/tools.hpp"
#include "routing/speed_camera_manager.hpp"
#include "routing/speed_camera_ser_des.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"

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

std::shared_ptr<Route> MakeSpeedCameraQueueRoute(double endLongitude)
{
  std::vector<m2::PointD> const points = {{0, 0}, {0, 0}, {0.001, 0}, {endLongitude, 0}, {endLongitude, 0}};
  std::vector<RouteSegment> segments;
  RouteSegmentsFrom({Segment{}, {0, 0, 0, true}, {0, 0, 1, true}, Segment{}}, points, {}, {}, segments);
  FillSegmentInfo({0, 0, 0, 0}, segments);
  segments[1].SetSpeedCameraInfo({{0.1, 50}, {0.5, 45}, {0.9, 40}});

  auto route = std::make_shared<Route>();
  route->SetGeometry(points.begin(), points.end());
  route->SetRouteSegments(std::move(segments));
  route->SetSubroutes(std::vector<Route::SubrouteAttrs>{
      {geometry::PointWithAltitude(points.front(), 0), geometry::PointWithAltitude(points.back(), 0), 0, 4}});
  return route;
}

UNIT_TEST(SpeedCamera_SkipsExpiredQueuedCameras)
{
  turns::sound::NotificationManager notifications;
  SpeedCameraManager manager(notifications);
  auto route = MakeSpeedCameraQueueRoute(0.01);
  route->GetRouteSegments()[2].SetSpeedCameraInfo({{0.85, 30}});
  manager.SetRoute(route);
  size_t clearCount = 0;
  manager.SetSpeedCamClearCallback([&] { ++clearCount; });

  location::GpsInfo gps;
  gps.m_speed = measurement_utils::KmphToMps(90);
  manager.OnLocationPositionChanged(gps);
  TEST(manager.GetClosestCamForTests().IsValid(), ());
  TEST_EQUAL(manager.GetClosestCamForTests().m_maxSpeedKmH, 50, ());
  TEST_EQUAL(clearCount, 1, ());
  TEST_EQUAL(manager.ShouldPlayBeepSignal(), manager.Enable(), ());
  TEST(!manager.ShouldPlayBeepSignal(), ());

  // A GPS jump passes the highlighted camera and both queued cameras on the first road.
  gps.m_longitude = 0.008;
  TEST(route->MoveIterator(gps), ());
  manager.OnLocationPositionChanged(gps);
  TEST(manager.GetClosestCamForTests().IsValid(), ());
  TEST_EQUAL(manager.GetClosestCamForTests().m_maxSpeedKmH, 30, ());
  TEST_EQUAL(clearCount, 3, ());
  TEST_EQUAL(manager.ShouldPlayBeepSignal(), manager.Enable(), ());

  // The scan is exhausted; the selected camera remains available on the next update.
  manager.OnLocationPositionChanged(gps);
  TEST(manager.GetClosestCamForTests().IsValid(), ());
  TEST_EQUAL(manager.GetClosestCamForTests().m_maxSpeedKmH, 30, ());
  TEST_EQUAL(clearCount, 3, ());
  TEST(!manager.ShouldPlayBeepSignal(), ());

  // Keep the selected camera while we are still inside its influence zone.
  gps.m_longitude = 0.009;
  TEST(route->MoveIterator(gps), ());
  manager.OnLocationPositionChanged(gps);
  TEST(manager.GetClosestCamForTests().IsValid(), ());
  TEST_EQUAL(manager.GetClosestCamForTests().m_maxSpeedKmH, 30, ());
  TEST_EQUAL(clearCount, 3, ());
  TEST(!manager.ShouldPlayBeepSignal(), ());
}

UNIT_TEST(SpeedCamera_ExpiredQueueKeepsDistantCamera)
{
  turns::sound::NotificationManager notifications;
  SpeedCameraManager manager(notifications);
  auto route = MakeSpeedCameraQueueRoute(0.02);
  route->GetRouteSegments()[2].SetSpeedCameraInfo({{0.9, 30}});
  manager.SetRoute(route);

  location::GpsInfo gps;
  gps.m_speed = 0;
  manager.OnLocationPositionChanged(gps);
  TEST(manager.GetClosestCamForTests().IsValid(), ());

  gps.m_longitude = 0.008;
  TEST(route->MoveIterator(gps), ());
  manager.OnLocationPositionChanged(gps);
  TEST(!manager.GetClosestCamForTests().IsValid(), ());
  manager.OnLocationPositionChanged(gps);
  TEST(!manager.GetClosestCamForTests().IsValid(), ());

  // The next camera was retained while it was more than 1 km ahead.
  gps.m_longitude = 0.013;
  TEST(route->MoveIterator(gps), ());
  manager.OnLocationPositionChanged(gps);
  TEST(manager.GetClosestCamForTests().IsValid(), ());
  TEST_EQUAL(manager.GetClosestCamForTests().m_maxSpeedKmH, 30, ());
}

UNIT_TEST(SpeedCamera_AllQueuedCamerasExpire)
{
  turns::sound::NotificationManager notifications;
  SpeedCameraManager manager(notifications);
  auto route = MakeSpeedCameraQueueRoute(0.01);
  manager.SetRoute(route);

  location::GpsInfo gps;
  gps.m_speed = 0;
  manager.OnLocationPositionChanged(gps);
  TEST(manager.GetClosestCamForTests().IsValid(), ());

  gps.m_longitude = 0.008;
  TEST(route->MoveIterator(gps), ());
  manager.OnLocationPositionChanged(gps);
  TEST(!manager.GetClosestCamForTests().IsValid(), ());
  manager.OnLocationPositionChanged(gps);
  TEST(!manager.GetClosestCamForTests().IsValid(), ());
}

UNIT_TEST(SpeedCamera_QueuedCameraAtInfluenceBoundary)
{
  turns::sound::NotificationManager notifications;
  SpeedCameraManager manager(notifications);
  auto route = MakeSpeedCameraQueueRoute(0.015);
  auto & segments = route->GetRouteSegments();
  segments[1].SetSpeedCameraInfo({{0.1, 50}});

  location::GpsInfo gps;
  gps.m_speed = 0;
  gps.m_longitude = 0.008;
  Route projectedRoute(*route);
  TEST(projectedRoute.MoveIterator(gps), ());
  double const passedDistance = projectedRoute.GetCurrentDistanceFromBeginMeters();

  // Synthetic cumulative meters place the camera exactly on the boundary without coefficient rounding.
  segments[1].SetDistancesAndTime(passedDistance - 450.0, segments[1].GetDistFromBeginningMerc(), 0);
  TEST_EQUAL(segments[1].GetDistFromBeginningMeters() - passedDistance, -450.0, ());
  segments[2].SetSpeedCameraInfo({{0, 40}, {0.55, 30}});
  manager.SetRoute(route);

  gps.m_longitude = 0;
  manager.OnLocationPositionChanged(gps);
  TEST(manager.GetClosestCamForTests().IsValid(), ());
  TEST_EQUAL(manager.GetClosestCamForTests().m_maxSpeedKmH, 50, ());

  // A camera exactly 450 m behind cannot be selected and must not block the next one.
  gps.m_longitude = 0.008;
  TEST(route->MoveIterator(gps), ());
  TEST_EQUAL(route->GetCurrentDistanceFromBeginMeters(), passedDistance, ());
  manager.OnLocationPositionChanged(gps);
  TEST(manager.GetClosestCamForTests().IsValid(), ());
  TEST_EQUAL(manager.GetClosestCamForTests().m_maxSpeedKmH, 30, ());
}
}  // namespace
