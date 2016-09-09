#include "testing/testing.hpp"

#include "map/chart_generator.hpp"

#include "base/math.hpp"

#include "std/vector.hpp"

namespace
{
double constexpr kEpsilon = 0.00001;

using namespace maps;

bool AlmostEqualAbs(vector<double> const & v1, vector<double> const & v2)
{
  if (v1.size() != v2.size())
    return false;

  for (size_t i = 0; i < v1.size(); ++i)
  {
    if (!my::AlmostEqualAbs(v1[i], v2[i], kEpsilon))
      return false;
  }
  return true;
}

bool IsColor(vector<uint8_t> const & frameBuffer, size_t startColorIdx, uint8_t expectedR,
             uint8_t expectedG, uint8_t expectedB, uint8_t expectedA)
{
  CHECK_LESS_OR_EQUAL(startColorIdx + kAlitudeChartBPP, frameBuffer.size(), ());

  return frameBuffer[startColorIdx] == expectedR && frameBuffer[startColorIdx + 1] == expectedG &&
         frameBuffer[startColorIdx + 2] == expectedB && frameBuffer[startColorIdx + 3] == expectedA;
}

void TestAngleColors(size_t width, size_t height, vector<uint8_t> const & frameBuffer,
                     uint8_t expectedR, uint8_t expectedG, uint8_t expectedB, uint8_t expectedA)
{
  TEST_EQUAL(frameBuffer.size(), width * height * kAlitudeChartBPP, ());
  TEST(IsColor(frameBuffer, 0 /* startColorIdx */, expectedR, expectedG, expectedB, expectedA), ());
  TEST(IsColor(frameBuffer, kAlitudeChartBPP * (width - 1) /* startColorIdx */, expectedR,
               expectedG, expectedB, expectedA), ());
  TEST(IsColor(frameBuffer, kAlitudeChartBPP * height * (width - 1) /* startColorIdx */,
               expectedR, expectedG, expectedB, expectedA), ());
  TEST(IsColor(frameBuffer, kAlitudeChartBPP * height * width - kAlitudeChartBPP /* startColorIdx */,
               expectedR, expectedG, expectedB, expectedA), ());
}

UNIT_TEST(NormalizeChartData_SmokeTest)
{
  vector<double> const distanceDataM = {0.0, 0.0, 0.0};
  feature::TAltitudes const altitudeDataM = {0, 0, 0};

  vector<double> uniformAltitudeDataM;
  TEST(maps::NormalizeChartData(distanceDataM, altitudeDataM, 2 /* resultPointCount */, uniformAltitudeDataM),
       ());

  vector<double> const expectedUniformAltitudeDataM = {0.0, 0.0};
  TEST_EQUAL(expectedUniformAltitudeDataM, uniformAltitudeDataM, ());
}

UNIT_TEST(NormalizeChartData_NoResultPointTest)
{
  vector<double> const distanceDataM = {0.0, 0.0, 0.0};
  feature::TAltitudes const altitudeDataM = {0, 0, 0};

  vector<double> uniformAltitudeDataM;
  TEST(maps::NormalizeChartData(distanceDataM, altitudeDataM, 0 /* resultPointCount */, uniformAltitudeDataM),
       ());

  TEST(uniformAltitudeDataM.empty(), ());
}

UNIT_TEST(NormalizeChartData_NoPointTest)
{
  vector<double> const distanceDataM = {};
  feature::TAltitudes const altitudeDataM = {};

  vector<double> uniformAltitudeDataM;
  TEST(maps::NormalizeChartData(distanceDataM, altitudeDataM, 2 /* resultPointCount */, uniformAltitudeDataM),
       ());

  TEST(uniformAltitudeDataM.empty(), ());
}

UNIT_TEST(NormalizeChartData_Test)
{
  vector<double> const distanceDataM = {0.0, 2.0, 4.0, 6.0};
  feature::TAltitudes const altitudeDataM = {-9, 0, 9, 18};

  vector<double> uniformAltitudeDataM;
  TEST(maps::NormalizeChartData(distanceDataM, altitudeDataM, 10 /* resultPointCount */, uniformAltitudeDataM),
       ());

  vector<double> const expectedUniformAltitudeDataM = {-9.0, -6.0, -3.0, 0.0,  3.0,
                                                       6.0,  9.0,  12.0, 15.0, 18.0};
  TEST(AlmostEqualAbs(uniformAltitudeDataM, expectedUniformAltitudeDataM), ());
}

UNIT_TEST(GenerateYAxisChartData_SmokeTest)
{
  vector<double> const altitudeDataM = {0.0, 0.0};
  vector<double> yAxisDataPxl;

  TEST(maps::GenerateYAxisChartData(30 /* height */, 1.0 /* minMetersPerPxl */, altitudeDataM, yAxisDataPxl), ());
  vector<double> expecttedYAxisDataPxl = {28.0, 28.0};
  TEST(AlmostEqualAbs(yAxisDataPxl, expecttedYAxisDataPxl), ());
}

UNIT_TEST(GenerateYAxisChartData_EmptyAltitudeDataTest)
{
  vector<double> const altitudeDataM = {};
  vector<double> yAxisDataPxl;

  TEST(maps::GenerateYAxisChartData(30 /* height */, 1.0 /* minMetersPerPxl */, altitudeDataM, yAxisDataPxl), ());
  TEST(yAxisDataPxl.empty(), ());
}

UNIT_TEST(GenerateYAxisChartData_Test)
{
  vector<double> const altitudeDataM = {0.0, 2.0, 0.0, -2.0, 1.0};
  vector<double> yAxisDataPxl;

  TEST(maps::GenerateYAxisChartData(100 /* height */, 1.0 /* minMetersPerPxl */, altitudeDataM, yAxisDataPxl), ());
  vector<double> expecttedYAxisDataPxl = {96.0, 94.0, 96.0, 98.0, 95.0};
  TEST(AlmostEqualAbs(yAxisDataPxl, expecttedYAxisDataPxl), ());
}

UNIT_TEST(GenerateChartByPoints_NoGeometryTest)
{
  vector<m2::PointD> const geometry = {};
  size_t constexpr width = 100;
  size_t constexpr height = 40;
  vector<uint8_t> frameBuffer;

  maps::GenerateChartByPoints(width, height, geometry, MapStyleLight /* mapStyle */, frameBuffer);
  TestAngleColors(width, height, frameBuffer, 255 /* expectedR */, 255 /* expectedG */,
                  255 /* expectedB */, 0 /* expectedA */);
}

UNIT_TEST(GenerateChartByPoints_OnePointTest)
{
  vector<m2::PointD> const geometry = {{20.0, 20.0}};
  size_t constexpr width = 40;
  size_t constexpr height = 40;
  vector<uint8_t> frameBuffer;

  maps::GenerateChartByPoints(width, height, geometry, MapStyleLight /* mapStyle */, frameBuffer);
  TestAngleColors(width, height, frameBuffer, 255 /* expectedR */, 255 /* expectedG */,
                  255 /* expectedB */, 0 /* expectedA */);
}

UNIT_TEST(GenerateChartByPoints_Test)
{
  vector<m2::PointD> const geometry = {{0.0, 0.0}, {10.0, 10.0}};

  size_t constexpr width = 40;
  size_t constexpr height = 40;
  vector<uint8_t> frameBuffer;

  maps::GenerateChartByPoints(width, height, geometry, MapStyleLight /* mapStyle */, frameBuffer);

  TEST_EQUAL(frameBuffer.size(), width * height * kAlitudeChartBPP, ());
  TEST(IsColor(frameBuffer, 0 /* startColorIdx */, 30 /* expectedR */, 150 /* expectedG */,
               240 /* expectedB */, 255 /* expectedA */),
       ());
  TEST(IsColor(frameBuffer, kAlitudeChartBPP * (width - 1) /* startColorIdx */, 255 /* expectedR */,
               255 /* expectedG */, 255 /* expectedB */, 0 /* expectedA */),
       ());
}

UNIT_TEST(GenerateChart_NoPointsTest)
{
  size_t constexpr width = 50;
  vector<double> const distanceDataM = {};
  feature::TAltitudes const & altitudeDataM = {};
  vector<uint8_t> frameBuffer;

  TEST(maps::GenerateChart(width, 50 /* height */, distanceDataM, altitudeDataM, MapStyleDark /* mapStyle */,
                           frameBuffer),
       ());
  TestAngleColors(width, 50 /* height */, frameBuffer, 255 /* expectedR */, 255 /* expectedG */,
                  255 /* expectedB */, 0 /* expectedA */);
}

UNIT_TEST(GenerateChart_OnePointTest)
{
  size_t constexpr width = 50;
  size_t constexpr height = 50;
  vector<double> const distanceDataM = {0.0};
  feature::TAltitudes const & altitudeDataM = {0};
  vector<uint8_t> frameBuffer;

  TEST(maps::GenerateChart(width, height, distanceDataM, altitudeDataM, MapStyleDark /* mapStyle */,
                           frameBuffer),
       ());
  TEST_EQUAL(frameBuffer.size(), width * height * kAlitudeChartBPP, ());
  TEST(IsColor(frameBuffer, 0 /* startColorIdx */, 255 /* expectedR */, 255 /* expectedG */,
               255 /* expectedB */, 0 /* expectedA */), ());
  TEST(IsColor(frameBuffer, kAlitudeChartBPP * (width - 1) /* startColorIdx */, 255 /* expectedR */,
               255 /* expectedG */, 255 /* expectedB */, 0 /* expectedA */), ());
}

UNIT_TEST(GenerateChart_EmptyRectTest)
{
  size_t constexpr width = 0;
  vector<double> const distanceDataM = {};
  feature::TAltitudes const & altitudeDataM = {};
  vector<uint8_t> frameBuffer;

  TEST(maps::GenerateChart(width, 50 /* height */, distanceDataM, altitudeDataM, MapStyleDark /* mapStyle */,
                           frameBuffer),
       ());
  TEST(frameBuffer.empty(), ());
}

UNIT_TEST(GenerateChart_Test)
{
  size_t constexpr width = 50;
  vector<double> const distanceDataM = {0.0, 100.0};
  feature::TAltitudes const & altitudeDataM = {0, 1000};
  vector<uint8_t> frameBuffer;

  TEST(maps::GenerateChart(width, 50 /* height */, distanceDataM, altitudeDataM, MapStyleDark /* mapStyle */,
                           frameBuffer),
       ());
  TEST(IsColor(frameBuffer, 0 /* startColorIdx */, 255 /* expectedR */, 255 /* expectedG */,
               255 /* expectedB */, 0 /* expectedA */),
       ());
  TEST(IsColor(frameBuffer, kAlitudeChartBPP * 3 * width -
               kAlitudeChartBPP /* startColorIdx */, 255 /* expectedR */,
               230 /* expectedG */, 140 /* expectedB */, 255 /* expectedA */),
       ());
}
}  // namespace
