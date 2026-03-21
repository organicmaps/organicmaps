#include "testing/testing.hpp"

#include "routing/speed_camera_ser_des.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/math.hpp"

#include <vector>

namespace
{
static auto const kTestDir = "ser_des_test_camera";
static auto const kTestFileForCamera = "ser_des_test_camera.bin";
static auto const kErrorMessageOneWay = "Serialize method works only with cameras with one way";

UNIT_TEST(SegmentCoord_LessOperator)
{
  {
    routing::SegmentCoord a(10, 5);
    routing::SegmentCoord b(11, 0);

    TEST_LESS(a, b, ());
  }

  {
    routing::SegmentCoord a(10, 0);
    routing::SegmentCoord b(11, 5);

    TEST_LESS(a, b, ());
  }

  {
    routing::SegmentCoord a(5, 0);
    routing::SegmentCoord b(11, 5);

    TEST_LESS(a, b, ());
  }

  {
    routing::SegmentCoord a(5, 0);
    routing::SegmentCoord b(0, 5);

    TEST_LESS(b, a, ());
  }

  {
    routing::SegmentCoord a(5, 0);
    routing::SegmentCoord b(5, 5);

    TEST_LESS(a, b, ());
  }

  {
    routing::SegmentCoord a(5, 6);
    routing::SegmentCoord b(5, 5);

    TEST_LESS(b, a, ());
  }

  {
    routing::SegmentCoord a(4, 6);
    routing::SegmentCoord b(5, 6);

    TEST_LESS(a, b, ());
  }
}

// Test for serialize/deserialize of speed cameras.

bool TestSerDesSpeedCamera(std::vector<routing::SpeedCameraMetadata> const & speedCamerasMetadata)
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

  platform::tests_support::ScopedDir scopedDir(kTestDir);
  auto const testFile = base::JoinPath(kTestDir, kTestFileForCamera);
  platform::tests_support::ScopedFile scopedFile(testFile, "");
  auto const & writableDir = GetPlatform().WritableDir();
  auto const & filePath = base::JoinPath(writableDir, testFile);

  {
    FileWriter writer(filePath);
    uint32_t prevFeatureId = 0;
    for (auto const & metadata : speedCamerasMetadata)
      routing::SerializeSpeedCamera(writer, metadata, prevFeatureId);
  }

  {
    FileReader reader(filePath);
    ReaderSource<FileReader> src(reader);
    uint32_t prevFeatureId = 0;
    for (auto const & metadata : speedCamerasMetadata)
    {
      auto const & way = metadata.m_ways.back();
      auto const res = routing::DeserializeSpeedCamera(src, prevFeatureId);
      TEST_EQUAL(res.first, routing::SegmentCoord(way.m_featureId, way.m_segmentId), ());
      TEST(AlmostEqualAbs(res.second.m_coef, way.m_coef, 1e-5), ());
      TEST_EQUAL(res.second.m_maxSpeedKmPH, metadata.m_maxSpeedKmPH, ());
    }
  }

  return true;
}

UNIT_TEST(SpeedCamera_SerDes_1)
{
  std::vector<routing::SpeedCameraMwmPosition> ways = {{1 /* featureId */, 1 /* segmentId */, 0 /* coef */}};
  routing::SpeedCameraMetadata metadata({10, 10} /* m_center */, 60 /* m_maxSpeedKmPH */, std::move(ways));

  TEST(TestSerDesSpeedCamera({metadata}), ());
}

UNIT_TEST(SpeedCamera_SerDes_2)
{
  std::vector<routing::SpeedCameraMetadata> speedCamerasMetadata;
  {
    std::vector<routing::SpeedCameraMwmPosition> ways = {{1 /* featureId */, 1 /* segmentId */, 0 /* coef */}};
    routing::SpeedCameraMetadata metadata({10, 10} /* m_center */, 60 /* m_maxSpeedKmPH */, std::move(ways));
    speedCamerasMetadata.emplace_back(metadata);
  }
  {
    std::vector<routing::SpeedCameraMwmPosition> ways = {{2 /* featureId */, 1 /* segmentId */, 0.5 /* coef */}};
    routing::SpeedCameraMetadata metadata({15, 10} /* m_center */, 90 /* m_maxSpeedKmPH */, std::move(ways));
    speedCamerasMetadata.emplace_back(metadata);
  }

  TEST(TestSerDesSpeedCamera(speedCamerasMetadata), ());
}

UNIT_TEST(SpeedCamera_SerDes_3)
{
  std::vector<routing::SpeedCameraMetadata> speedCamerasMetadata;
  {
    std::vector<routing::SpeedCameraMwmPosition> ways = {{1 /* featureId */, 1 /* segmentId */, 0 /* coef */}};
    routing::SpeedCameraMetadata metadata({10, 10} /* m_center */, 60 /* m_maxSpeedKmPH */, std::move(ways));
    speedCamerasMetadata.emplace_back(metadata);
  }
  {
    std::vector<routing::SpeedCameraMwmPosition> ways = {{1 /* featureId */, 1 /* segmentId */, 0.5 /* coef */}};
    routing::SpeedCameraMetadata metadata({10, 10} /* m_center */, 90 /* m_maxSpeedKmPH */, std::move(ways));
    speedCamerasMetadata.emplace_back(metadata);
  }
  {
    std::vector<routing::SpeedCameraMwmPosition> ways = {{10 /* featureId */, 1 /* segmentId */, 1 /* coef */}};
    routing::SpeedCameraMetadata metadata({20, 20} /* m_center */, 40 /* m_maxSpeedKmPH */, std::move(ways));
    speedCamerasMetadata.emplace_back(metadata);
  }

  TEST(TestSerDesSpeedCamera(speedCamerasMetadata), ());
}
}  // namespace
