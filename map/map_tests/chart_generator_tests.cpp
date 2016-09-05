#include "testing/testing.hpp"

#include "map/chart_generator.hpp"

#include "base/math.hpp"

#include "std/deque.hpp"
#include "std/vector.hpp"

namespace
{
double constexpr kEpsilon = 0.00001;
uint32_t constexpr kBPP = 4;

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
  CHECK_LESS(startColorIdx + kBPP, frameBuffer.size(), ());

  return frameBuffer[startColorIdx] == expectedR && frameBuffer[startColorIdx + 1] == expectedG &&
         frameBuffer[startColorIdx + 2] == expectedB && frameBuffer[startColorIdx + 3] == expectedA;
}

UNIT_TEST(NormalizeChartDataZeroTest)
{
  deque<double> const distanceDataM = {0.0, 0.0, 0.0};
  feature::TAltitudes const altitudeDataM = {0, 0, 0};

  vector<double> uniformAltitudeDataM;
  NormalizeChartData(distanceDataM, altitudeDataM, 2 /* resultPointCount */, uniformAltitudeDataM);

  vector<double> const expectedUniformAltitudeDataM = {0.0, 0.0};
  TEST_EQUAL(expectedUniformAltitudeDataM, uniformAltitudeDataM, ());
}

UNIT_TEST(NormalizeChartDataTest)
{
  deque<double> const distanceDataM = {0.0, 2.0, 4.0, 6.0};
  feature::TAltitudes const altitudeDataM = {-9, 0, 9, 18};

  vector<double> uniformAltitudeDataM;
  NormalizeChartData(distanceDataM, altitudeDataM, 10 /* resultPointCount */, uniformAltitudeDataM);

  vector<double> const expectedUniformAltitudeDataM = {-9.0, -6.0, -3.0, 0.0,  3.0,
                                                       6.0,  9.0,  12.0, 15.0, 18.0};
  TEST(AlmostEqualAbs(uniformAltitudeDataM, expectedUniformAltitudeDataM), ());
}

UNIT_TEST(GenerateYAxisChartDataZeroTest)
{
  vector<double> const altitudeDataM = {0.0, 0.0};
  vector<double> yAxisDataPxl;

  GenerateYAxisChartData(30 /* height */, 1.0 /* minMetersPerPxl */, altitudeDataM, yAxisDataPxl);
  vector<double> expecttedYAxisDataPxl = {28.0, 28.0};
  TEST(AlmostEqualAbs(yAxisDataPxl, expecttedYAxisDataPxl), ());
}

UNIT_TEST(GenerateYAxisChartDataTest)
{
  vector<double> const altitudeDataM = {0.0, 2.0, 0.0, -2.0, 1.0};
  vector<double> yAxisDataPxl;

  GenerateYAxisChartData(100 /* height */, 1.0 /* minMetersPerPxl */, altitudeDataM, yAxisDataPxl);
  vector<double> expecttedYAxisDataPxl = {96.0, 94.0, 96.0, 98.0, 95.0};
  TEST(AlmostEqualAbs(yAxisDataPxl, expecttedYAxisDataPxl), ());
}

UNIT_TEST(GenerateChartByPointsTest)
{
  vector<m2::PointD> const geometry = {{0.0, 0.0}, {10.0, 10.0}};

  size_t constexpr width = 40;
  vector<uint8_t> frameBuffer;

  GenerateChartByPoints(width, 40 /* height */, geometry, true /* day */, frameBuffer);
  TEST(IsColor(frameBuffer, 0 /* startColorIdx */, 30 /* expectedR */, 150 /* expectedG */,
               240 /* expectedB */, 255 /* expectedA */),
       ());
  TEST(IsColor(frameBuffer, kBPP * (width - 1) /* startColorIdx */, 255 /* expectedR */,
               255 /* expectedG */, 255 /* expectedB */, 0 /* expectedA */),
       ());
}

UNIT_TEST(GenerateChartTest)
{
  size_t constexpr width = 50;
  deque<double> const distanceDataM = {0.0, 100.0};
  feature::TAltitudes const & altitudeDataM = {0, 1000};
  vector<uint8_t> frameBuffer;

  GenerateChart(width, 50 /* height */, distanceDataM, altitudeDataM, false /* day */, frameBuffer);
  TEST(IsColor(frameBuffer, 0 /* startColorIdx */, 255 /* expectedR */, 255 /* expectedG */,
               255 /* expectedB */, 0 /* expectedA */),
       ());
  TEST(IsColor(frameBuffer, kBPP * 3 * width - kBPP /* startColorIdx */, 255 /* expectedR */,
               230 /* expectedG */, 140 /* expectedB */, 255 /* expectedA */),
       ());
}
}  // namespace
