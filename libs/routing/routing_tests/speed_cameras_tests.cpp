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

#include <memory>
#include <string>
#include <string_view>
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

std::shared_ptr<Route> MakeModeChangeCameraRoute(uint8_t speedLimit)
{
  std::vector<m2::PointD> const points = {{0, 0}, {0, 0}, {0.0048, 0}, {0.0058, 0}, {0.02, 0}, {0.02, 0}};
  std::vector<RouteSegment> segments;
  RouteSegmentsFrom({Segment{}, {0, 0, 0, true}, {0, 0, 1, true}, {0, 0, 2, true}, Segment{}}, points, {}, {},
                    segments);
  FillSegmentInfo({0, 0, 0, 0, 0}, segments);
  segments[2].SetSpeedCameraInfo({{0.5, speedLimit}});

  auto route = std::make_shared<Route>();
  route->SetGeometry(points.begin(), points.end());
  route->SetRouteSegments(std::move(segments));
  route->SetSubroutes(std::vector<Route::SubrouteAttrs>{
      {geometry::PointWithAltitude(points.front(), 0), geometry::PointWithAltitude(points.back(), 0), 0, 5}});
  return route;
}

UNIT_TEST(SpeedCamera_ModeChangesRefreshTrackedMarker)
{
  std::string_view constexpr kModeKey = "speed_cam_mode";
  std::string savedMode;
  bool const hadSetting = settings::Get(kModeKey, savedMode);
  SCOPE_GUARD(restoreMode, [&]
  {
    if (hadSetting)
      settings::Set(kModeKey, savedMode);
    else
      settings::Delete(kModeKey);
  });
  settings::Delete(kModeKey);

  for (uint8_t const limit : {uint8_t{40}, SpeedCameraOnRoute::kNoSpeedInfo})
  {
    for (auto const initialMode : {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Never})
    {
      bool const initiallyVisible = initialMode == SpeedCameraManagerMode::Auto;
      bool markerVisible = false;
      size_t shows = 0;
      size_t clears = 0;
      m2::PointD shownPoint;
      double shownSpeed = 0;
      turns::sound::NotificationManager notifications;
      notifications.SetLocaleWithJsonForTesting(R"({"unknown_camera":"camera"})", "en");
      SpeedCameraManager manager(notifications);
      manager.SetSpeedCamClearCallback([&]
      {
        ++clears;
        markerVisible = false;
      });
      manager.SetSpeedCamShowCallback([&](m2::PointD const & point, double speed)
      {
        ++shows;
        markerVisible = true;
        shownPoint = point;
        shownSpeed = speed;
      });
      manager.SetMode(initialMode);
      TEST_EQUAL(clears, 0, ());
      TEST_EQUAL(shows, 0, ());

      auto route = MakeModeChangeCameraRoute(limit);
      manager.SetRoute(route);
      location::GpsInfo gps;
      gps.m_speed = measurement_utils::KmphToMps(90);
      manager.OnLocationPositionChanged(gps);
      auto const selectedCamera = manager.GetClosestCamForTests();
      TEST(selectedCamera.IsValid(), ());
      TEST_EQUAL(markerVisible, initiallyVisible, ());
      TEST_EQUAL(clears, 1, ());
      TEST_EQUAL(shows, initiallyVisible ? 1 : 0, ());

      // The camera is in the voice zone for both known and unknown limits.
      std::vector<std::string> texts;
      manager.GenerateNotifications(texts);
      TEST_EQUAL(texts.size(), initiallyVisible ? 1 : 0, ());
      if (!texts.empty())
        TEST_EQUAL(texts.front(), "camera", ());
      TEST(!manager.ShouldPlayBeepSignal(), ());

      auto const mode = initiallyVisible ? SpeedCameraManagerMode::Never : SpeedCameraManagerMode::Always;
      manager.SetMode(mode);
      TEST_EQUAL(markerVisible, !initiallyVisible, ());
      TEST_EQUAL(clears, 2, ());
      TEST_EQUAL(shows, 1, ());
      TEST_ALMOST_EQUAL_ABS(shownPoint.x, 0.0053, 1e-12, ());
      TEST_EQUAL(shownPoint.y, 0, ());
      TEST_EQUAL(shownSpeed, limit, ());
      SpeedCameraManager reloaded(notifications);
      TEST_EQUAL(reloaded.GetMode(), mode, ());

      manager.SetMode(mode);
      manager.OnLocationPositionChanged(gps);
      TEST_EQUAL(clears, 2, ());
      TEST_EQUAL(shows, 1, ());
      TEST_EQUAL(markerVisible, !initiallyVisible, ());
      TEST_EQUAL(manager.GetClosestCamForTests().m_position, selectedCamera.m_position, ());
      texts.clear();
      manager.GenerateNotifications(texts);
      TEST_EQUAL(texts.size(), initiallyVisible ? 0 : 1, ());
      TEST(!manager.ShouldPlayBeepSignal(), ());

      // Marker refreshes keep the one-shot notification state for the selected camera.
      manager.SetMode(SpeedCameraManagerMode::Never);
      manager.SetMode(SpeedCameraManagerMode::Always);
      manager.OnLocationPositionChanged(gps);
      texts.clear();
      manager.GenerateNotifications(texts);
      TEST(texts.empty(), ());
      TEST(!manager.ShouldPlayBeepSignal(), ());

      gps.m_longitude = 0.0048;
      TEST(route->MoveIterator(gps), ());
      manager.OnLocationPositionChanged(gps);
      TEST(manager.ShouldPlayBeepSignal(), ());
      TEST(!manager.ShouldPlayBeepSignal(), ());
      manager.SetMode(SpeedCameraManagerMode::Never);
      manager.SetMode(SpeedCameraManagerMode::Always);
      manager.OnLocationPositionChanged(gps);
      TEST(!manager.ShouldPlayBeepSignal(), ());
      texts.clear();
      manager.GenerateNotifications(texts);
      TEST(texts.empty(), ());

      manager.Reset();
      auto const clearsAfterReset = clears;
      auto const showsAfterReset = shows;
      manager.SetMode(SpeedCameraManagerMode::Never);
      TEST_EQUAL(clears, clearsAfterReset, ());
      TEST_EQUAL(shows, showsAfterReset, ());
    }
  }
}
}  // namespace
