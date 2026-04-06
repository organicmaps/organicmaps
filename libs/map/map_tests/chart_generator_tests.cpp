#include "testing/testing.hpp"

#include "map/chart_generator.hpp"
#include "map/elevation_info.hpp"

#include "base/math.hpp"

#include <vector>

namespace chart_generator_tests
{
using std::vector;

namespace
{
bool AlmostEqualAbs(vector<double> const & v1, vector<double> const & v2)
{
  if (v1.size() != v2.size())
    return false;

  double constexpr kEpsilon = 1.0E-6;
  for (size_t i = 0; i < v1.size(); ++i)
    if (!::AlmostEqualAbs(v1[i], v2[i], kEpsilon))
      return false;
  return true;
}

bool IsColor(vector<uint8_t> const & frameBuffer, size_t startColorIdx, uint8_t expectedR, uint8_t expectedG,
             uint8_t expectedB, uint8_t expectedA)
{
  CHECK_LESS_OR_EQUAL(startColorIdx + ChartGenerator::kBPP, frameBuffer.size(), ());

  return frameBuffer[startColorIdx] == expectedR && frameBuffer[startColorIdx + 1] == expectedG &&
         frameBuffer[startColorIdx + 2] == expectedB && frameBuffer[startColorIdx + 3] == expectedA;
}

void TestAngleColors(size_t width, size_t height, vector<uint8_t> const & frameBuffer, uint8_t expectedR,
                     uint8_t expectedG, uint8_t expectedB, uint8_t expectedA)
{
  TEST_EQUAL(frameBuffer.size(), width * height * ChartGenerator::kBPP, ());
  TEST(IsColor(frameBuffer, 0 /* startColorIdx */, expectedR, expectedG, expectedB, expectedA), ());
  TEST(IsColor(frameBuffer, ChartGenerator::kBPP * (width - 1) /* startColorIdx */, expectedR, expectedG, expectedB,
               expectedA),
       ());
  TEST(IsColor(frameBuffer, ChartGenerator::kBPP * height * (width - 1) /* startColorIdx */, expectedR, expectedG,
               expectedB, expectedA),
       ());
  TEST(IsColor(frameBuffer, ChartGenerator::kBPP * height * width - ChartGenerator::kBPP /* startColorIdx */, expectedR,
               expectedG, expectedB, expectedA),
       ());
}

ElevationInfo MakeElevationInfo(vector<double> const & distances, geometry::Altitudes const & altitudes)
{
  TEST_EQUAL(distances.size() + 1, altitudes.size(), ());
  ElevationInfo info;
  info.Assign(distances, altitudes);
  return info;
}
}  // namespace

UNIT_TEST(ScaleChartData_Test)
{
  vector<double> chartData = {0.0, -1.0, 2.0};
  ChartGenerator::ScaleChartData(chartData, 2.0 /* scale */);
  vector<double> const expectedChartData = {0.0, -2.0, 4.0};
  TEST_EQUAL(chartData, expectedChartData, ());
}

UNIT_TEST(ShiftChartData_Test)
{
  vector<double> chartData = {0.0, -1.0, 2.0};
  ChartGenerator::ShiftChartData(chartData, 1 /* shift */);
  vector<double> const expectedChartData = {1.0, 0.0, 3.0};
  TEST_EQUAL(chartData, expectedChartData, ());
}

UNIT_TEST(ReflectChartData_Test)
{
  vector<double> chartData = {0.0, -1.0, 2.0};
  ChartGenerator::ReflectChartData(chartData);
  vector<double> const expectedChartData = {0.0, 1.0, -2.0};
  TEST_EQUAL(chartData, expectedChartData, ());
}

UNIT_TEST(GenerateYAxisChartData_SmokeTest)
{
  vector<double> const altitudeDataM = {0.0, 0.0};
  vector<double> yAxisDataPxl;

  TEST(ChartGenerator::GenerateYAxisChartData(30 /* height */, 1.0 /* minMetersPerPxl */, altitudeDataM, yAxisDataPxl),
       ());
  TEST(AlmostEqualAbs(yAxisDataPxl, {15.0, 15.0}), ());
}

UNIT_TEST(GenerateYAxisChartData_Test)
{
  vector<double> const altitudeDataM = {0.0, 2.0, 0.0, -2.0, 1.0};
  vector<double> yAxisDataPxl;

  TEST(ChartGenerator::GenerateYAxisChartData(100 /* height */, 1.0 /* minMetersPerPxl */, altitudeDataM, yAxisDataPxl),
       ());
  TEST(AlmostEqualAbs(yAxisDataPxl, {50.0, 48.0, 50.0, 52.0, 49.0}), ());
}

UNIT_TEST(GenerateChartByPoints_NoGeometryTest)
{
  vector<m2::PointD> const geometry = {};
  size_t constexpr width = 100;
  size_t constexpr height = 40;
  vector<uint8_t> frameBuffer;

  ChartGenerator::GenerateByPoints(width, height, geometry, MapStyleDefaultLight /* mapStyle */, frameBuffer);
  TestAngleColors(width, height, frameBuffer, 255 /* expectedR */, 255 /* expectedG */, 255 /* expectedB */,
                  0 /* expectedA */);
}

UNIT_TEST(GenerateChartByPoints_OnePointTest)
{
  vector<m2::PointD> const geometry = {{20.0, 20.0}};
  size_t constexpr width = 40;
  size_t constexpr height = 40;
  vector<uint8_t> frameBuffer;

  ChartGenerator::GenerateByPoints(width, height, geometry, MapStyleDefaultLight /* mapStyle */, frameBuffer);
  TestAngleColors(width, height, frameBuffer, 255 /* expectedR */, 255 /* expectedG */, 255 /* expectedB */,
                  0 /* expectedA */);
}

UNIT_TEST(GenerateChartByPoints_Test)
{
  vector<m2::PointD> const geometry = {{0.0, 0.0}, {10.0, 10.0}};

  size_t constexpr width = 40;
  size_t constexpr height = 40;
  vector<uint8_t> frameBuffer;

  ChartGenerator::GenerateByPoints(width, height, geometry, MapStyleDefaultLight /* mapStyle */, frameBuffer);

  TEST_EQUAL(frameBuffer.size(), width * height * ChartGenerator::kBPP, ());
  TEST(IsColor(frameBuffer, 0 /* startColorIdx */, 30 /* expectedR */, 150 /* expectedG */, 240 /* expectedB */,
               255 /* expectedA */),
       ());
  TEST(IsColor(frameBuffer, ChartGenerator::kBPP * (width - 1) /* startColorIdx */, 255 /* expectedR */,
               255 /* expectedG */, 255 /* expectedB */, 0 /* expectedA */),
       ());
}

UNIT_TEST(ChartGenerator_NormalizeAltitudes_Smoke)
{
  // distances: 0, 2, 4, 6; altitudes: -9, 0, 9, 18 (linear)
  auto const info = MakeElevationInfo({2.0, 4.0, 6.0}, {-9, 0, 9, 18});

  vector<double> uniformAltitudeDataM;
  ChartGenerator(info).NormalizeAltitudes(10 /* resultPointCount */, uniformAltitudeDataM);
  TEST(AlmostEqualAbs(uniformAltitudeDataM, {-9.0, -6.0, -3.0, 0.0, 3.0, 6.0, 9.0, 12.0, 15.0, 18.0}), ());
}

UNIT_TEST(ChartGenerator_NormalizeAltitudes_ZeroLength)
{
  // distances: 0, 0; altitudes: 5, 10
  auto const info = MakeElevationInfo({0.0}, {5, 10});

  {
    vector<double> uniformAltitudeDataM;
    ChartGenerator(info).NormalizeAltitudes(1 /* resultPointCount */, uniformAltitudeDataM);
    TEST(AlmostEqualAbs(uniformAltitudeDataM, {10.0}), ());
  }

  {
    vector<double> uniformAltitudeDataM;
    ChartGenerator(info).NormalizeAltitudes(2 /* resultPointCount */, uniformAltitudeDataM);
    TEST(AlmostEqualAbs(uniformAltitudeDataM, {5.0, 10.0}), ());
  }
}

UNIT_TEST(ChartGenerator_SinglePoint)
{
  // Single point at altitude 100 (zero length).
  auto const info = MakeElevationInfo({} /* distances */, {100} /* altitudes */);

  size_t constexpr width = 50;
  size_t constexpr height = 50;
  vector<uint8_t> frameBuffer;

  TEST(ChartGenerator(info).Generate(width, height, MapStyleDefaultLight, frameBuffer), ());
  TEST_EQUAL(frameBuffer.size(), width * height * ChartGenerator::kBPP, ());
}

UNIT_TEST(ChartGenerator_LinearAscent)
{
  // 0m→1000m over 100m distance, 4 points.
  auto const info = MakeElevationInfo({30, 70, 100}, {0, 300, 700, 1000});

  size_t constexpr width = 50;
  size_t constexpr height = 50;
  vector<uint8_t> frameBuffer;

  TEST(ChartGenerator(info).Generate(width, height, MapStyleDefaultDark, frameBuffer), ());
  TEST_EQUAL(frameBuffer.size(), width * height * ChartGenerator::kBPP, ());

  // Top-right corner should be transparent background (chart goes from bottom-left to top-right).
  TEST(IsColor(frameBuffer, ChartGenerator::kBPP * (width - 1) /* startColorIdx */, 255 /* expectedR */,
               255 /* expectedG */, 255 /* expectedB */, 0 /* expectedA */),
       ());
}

UNIT_TEST(ChartGenerator_FlatLine)
{
  // Flat line at altitude 500 over 200m.
  auto const info = MakeElevationInfo({100, 200}, {500, 500, 500});

  size_t constexpr width = 40;
  size_t constexpr height = 40;
  vector<uint8_t> frameBuffer;

  TEST(ChartGenerator(info).Generate(width, height, MapStyleDefaultLight, frameBuffer), ());
  TEST_EQUAL(frameBuffer.size(), width * height * ChartGenerator::kBPP, ());
}
}  // namespace chart_generator_tests
