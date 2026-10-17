#include "testing/testing.hpp"

#include "routing/routing_helpers.hpp"
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

UNIT_TEST(SpeedCamera_TraversalKeepsPositionsAndTravelOrder)
{
  for (bool const forward : {true, false})
  {
    for (bool const atEndpoints : {false, true})
    {
      auto route = std::make_shared<Route>();
      // The leading road has no cameras, so both camera distances include its length.
      std::vector<m2::PointD> const points =
          forward ? std::vector<m2::PointD>{{-0.001, 0}, {-0.001, 0}, {0, 0}, {0.001, 0}, {0.003, 0}}
                  : std::vector<m2::PointD>{{0.002, 0}, {0.002, 0}, {0.001, 0}, {0, 0}, {-0.002, 0}};
      route->SetGeometry(points.begin(), points.end());
      std::vector<RouteSegment> segments;
      for (uint32_t i = 0; i < 4; ++i)
        segments.emplace_back(i == 0 || i == 3 ? Segment() : Segment(0, i, 0, forward), turns::TurnItem{},
                              geometry::PointWithAltitude(points[i + 1], 0), RouteSegment::RoadNameInfo{});
      FillSegmentInfo({0, 1, 2, 3}, segments);
      double const lowCoef = atEndpoints ? 0.0 : 0.25;
      double const highCoef = atEndpoints ? 1.0 : 0.75;
      // The loader orders events by ascending coefficient forward, descending backward.
      segments[2].SetSpeedCameraInfo({{forward ? lowCoef : highCoef, 40}, {forward ? highCoef : lowCoef, 50}});
      route->SetRouteSegments(std::move(segments));
      route->SetSubroutes(std::vector<Route::SubrouteAttrs>{
          {geometry::PointWithAltitude(points.front(), 0), geometry::PointWithAltitude(points.back(), 0), 0, 4}});
      turns::sound::NotificationManager notifications;
      SpeedCameraManager manager(notifications);
      manager.SetRoute(route);
      location::GpsInfo gps;
      gps.m_speed = 0;
      manager.OnLocationPositionChanged(gps);
      auto const first = manager.GetClosestCamForTests();
      TEST(first.IsValid(), ());
      double const lowPosition = atEndpoints ? 0.0 : 0.00025;
      double const highPosition = atEndpoints ? 0.001 : 0.00075;
      TEST_ALMOST_EQUAL_ABS(first.m_position.x, forward ? lowPosition : highPosition, 1e-12, ());
      TEST_EQUAL(first.m_position.y, 0.0, ());
      TEST_EQUAL(first.m_maxSpeedKmH, 40, ());
      TEST_ALMOST_EQUAL_ABS(first.m_distFromBeginMeters,
                            mercator::DistanceOnEarth(points.front(), {forward ? lowPosition : highPosition, 0}), 1e-8,
                            ());

      gps.m_longitude = forward ? 0.0015 : -0.0005;
      gps.m_latitude = 0;
      gps.m_horizontalAccuracy = 5;
      TEST(route->MoveIterator(gps), ());
      manager.OnLocationPositionChanged(gps);
      auto const & second = manager.GetClosestCamForTests();
      TEST(second.IsValid(), ());
      TEST_ALMOST_EQUAL_ABS(second.m_position.x, forward ? highPosition : lowPosition, 1e-12, ());
      TEST_EQUAL(second.m_position.y, 0.0, ());
      TEST_EQUAL(second.m_maxSpeedKmH, 50, ());
      TEST_ALMOST_EQUAL_ABS(second.m_distFromBeginMeters,
                            mercator::DistanceOnEarth(points.front(), {forward ? highPosition : lowPosition, 0}), 1e-8,
                            ());
    }
  }
}
}  // namespace
