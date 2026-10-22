#include "testing/testing.hpp"

#include "routing/routing_helpers.hpp"
#include "routing/routing_tests/tools.hpp"
#include "routing/speed_camera_manager.hpp"
#include "routing/speed_camera_ser_des.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"

#include <cstdint>
#include <memory>
#include <string>
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

auto PreserveCameraStateModeSetting()
{
  std::string value;
  bool const stored = settings::Get("speed_cam_mode", value);
  return base::make_scope_guard([stored, value]
  {
    if (stored)
      settings::Set("speed_cam_mode", value);
    else
      settings::Delete("speed_cam_mode");
  });
}

std::shared_ptr<Route> MakeCameraStateRoute(std::vector<RouteSegment::SpeedCamera> cameras)
{
  std::vector<m2::PointD> const points = {{0, 0}, {0, 0}, {0.001, 0}, {0.02, 0}, {0.02, 0}};
  std::vector<RouteSegment> segments;
  RouteSegmentsFrom({Segment{}, {0, 0, 0, true}, {0, 0, 1, true}, Segment{}}, points, {}, {}, segments);
  FillSegmentInfo({0, 0, 0, 0}, segments);
  segments[2].SetSpeedCameraInfo(std::move(cameras));

  auto route = std::make_shared<Route>();
  route->SetGeometry(points.begin(), points.end());
  route->SetRouteSegments(std::move(segments));
  route->SetSubroutes(std::vector<Route::SubrouteAttrs>{
      {geometry::PointWithAltitude(points.front(), 0), geometry::PointWithAltitude(points.back(), 0), 0, 4}});
  return route;
}

void TestCameraStateSignalExpiry(bool voice)
{
  auto const restoreMode = PreserveCameraStateModeSetting();
  settings::Delete("speed_cam_mode");
  for (auto const mode : {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always})
  {
    for (bool const expired : {false, true})
    {
      turns::sound::NotificationManager notifications;
      notifications.SetLocaleWithJsonForTesting(R"({"unknown_camera":"camera"})", "en");
      SpeedCameraManager manager(notifications);
      manager.SetMode(mode);
      auto route = MakeCameraStateRoute({{voice ? 0.3 : 0.1, 30}});
      manager.SetRoute(route);
      location::GpsInfo gps;
      gps.m_speed = measurement_utils::KmphToMps(180);
      manager.OnLocationPositionChanged(gps);
      TEST(manager.GetClosestCamForTests().IsValid(), (mode, voice, expired));

      if (expired)
      {
        // Defer consumption until the selected camera is beyond its rear influence zone.
        gps.m_longitude = 0.018;
        TEST(route->MoveIterator(gps), ());
        manager.OnLocationPositionChanged(gps);
        TEST(!manager.GetClosestCamForTests().IsValid(), (mode, voice));
      }

      std::vector<std::string> texts;
      manager.GenerateNotifications(texts);
      TEST_EQUAL(texts, (!expired && voice ? std::vector<std::string>{"camera"} : std::vector<std::string>{}),
                 (mode, voice, expired));
      bool const beep = manager.ShouldPlayBeepSignal();
      TEST_EQUAL(beep, !expired && !voice, (mode, voice, expired));
      texts.clear();
      manager.OnLocationPositionChanged(gps);
      manager.GenerateNotifications(texts);
      TEST(texts.empty(), (mode, voice, expired));
      TEST(!manager.ShouldPlayBeepSignal(), (mode, voice, expired));
    }
  }
}

UNIT_TEST(SpeedCamera_ExpiresUnconsumedBeep)
{
  TestCameraStateSignalExpiry(false);
}

UNIT_TEST(SpeedCamera_ExpiresUnconsumedVoice)
{
  TestCameraStateSignalExpiry(true);
}

UNIT_TEST(SpeedCamera_UpdatesSpeedLimitOnSelection)
{
  auto const restoreMode = PreserveCameraStateModeSetting();
  settings::Delete("speed_cam_mode");
  for (auto const mode : {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always, SpeedCameraManagerMode::Never})
  {
    for (auto const limit : {uint8_t{50}, SpeedCameraOnRoute::kNoSpeedInfo})
    {
      for (double const speedKmH : {20.0, 90.0})
      {
        turns::sound::NotificationManager notifications;
        SpeedCameraManager manager(notifications);
        manager.SetMode(mode);
        auto route = MakeCameraStateRoute({{0.1, limit}});
        manager.SetRoute(route);
        location::GpsInfo gps;
        gps.m_speed = measurement_utils::KmphToMps(speedKmH);
        manager.OnLocationPositionChanged(gps);
        TEST(manager.GetClosestCamForTests().IsValid(), ());
        bool const expected = limit == 50 && speedKmH > 50;
        TEST_EQUAL(manager.IsSpeedLimitExceeded(), expected, (mode, limit, speedKmH));
        manager.OnLocationPositionChanged(gps);
        TEST_EQUAL(manager.IsSpeedLimitExceeded(), expected, (mode, limit, speedKmH));
      }
    }
  }
}

UNIT_TEST(SpeedCamera_UpdatesSpeedLimitAndQuotaOnPromotion)
{
  auto const restoreMode = PreserveCameraStateModeSetting();
  settings::Delete("speed_cam_mode");
  for (auto const mode : {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always})
  {
    turns::sound::NotificationManager notifications;
    SpeedCameraManager manager(notifications);
    manager.SetMode(mode);
    auto route = MakeCameraStateRoute({{0.1, 50}, {0.3, 30}});
    manager.SetRoute(route);
    location::GpsInfo gps;
    gps.m_speed = measurement_utils::KmphToMps(90);
    manager.OnLocationPositionChanged(gps);
    TEST_EQUAL(manager.GetClosestCamForTests().m_maxSpeedKmH, 50, ());
    TEST(manager.ShouldPlayBeepSignal(), (mode));
    manager.OnLocationPositionChanged(gps);
    TEST(!manager.ShouldPlayBeepSignal(), (mode));

    gps.m_longitude = 0.004;
    TEST(route->MoveIterator(gps), ());
    manager.OnLocationPositionChanged(gps);
    TEST_EQUAL(manager.GetClosestCamForTests().m_maxSpeedKmH, 30, ());
    TEST(manager.IsSpeedLimitExceeded(), (mode));
    TEST(manager.ShouldPlayBeepSignal(), (mode));
    manager.OnLocationPositionChanged(gps);
    TEST(manager.IsSpeedLimitExceeded(), (mode));
    TEST(!manager.ShouldPlayBeepSignal(), (mode));
  }
}
}  // namespace
