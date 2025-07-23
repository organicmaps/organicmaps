#include "testing/testing.hpp"

#include "coding/byte_stream.hpp"
#include "coding/coding_tests/test_polylines.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "geometry/geometry_tests/large_polygon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"
#include "geometry/simplification.hpp"

#include "base/logging.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace coding;
using namespace std;

using PU = m2::PointU;
using PD = m2::PointD;

namespace
{
m2::PointU D2U(m2::PointD const & p)
{
  return PointDToPointU(p, kPointCoordBits);
}

m2::PointU GetMaxPoint()
{
  return D2U(m2::PointD(mercator::Bounds::kMaxX, mercator::Bounds::kMaxY));
}

void TestPolylineEncode(string testName, vector<m2::PointU> const & points, m2::PointU const & maxPoint,
                        void (*fnEncode)(InPointsT const & points, m2::PointU const & basePoint,
                                         m2::PointU const & maxPoint, OutDeltasT & deltas),
                        void (*fnDecode)(InDeltasT const & deltas, m2::PointU const & basePoint,
                                         m2::PointU const & maxPoint, OutPointsT & points))
{
  size_t const count = points.size();
  if (count == 0)
    return;

  m2::PointU const basePoint = m2::PointU::Zero();

  vector<uint64_t> deltas;
  deltas.resize(count);

  OutDeltasT deltasA(deltas);
  fnEncode(make_read_adapter(points), basePoint, maxPoint, deltasA);

  vector<m2::PointU> decodedPoints;
  decodedPoints.resize(count);

  OutPointsT decodedPointsA(decodedPoints);
  fnDecode(make_read_adapter(deltas), basePoint, maxPoint, decodedPointsA);

  TEST_EQUAL(points, decodedPoints, ());

  if (points.size() > 10)
  {
    vector<char> data;
    MemWriter<vector<char>> writer(data);

    for (size_t i = 0; i != deltas.size(); ++i)
      WriteVarUint(writer, deltas[i]);

    LOG(LINFO, (testName, points.size(), data.size()));
  }
}

vector<m2::PointU> SimplifyPoints(vector<m2::PointU> const & points, double eps)
{
  vector<m2::PointU> simpPoints;
  SimplifyDefault(points.begin(), points.end(), eps, simpPoints);
  return simpPoints;
}

void TestEncodePolyline(string name, m2::PointU maxPoint, vector<m2::PointU> const & points)
{
  TestPolylineEncode(name + "1", points, maxPoint, &EncodePolylinePrev1, &DecodePolylinePrev1);
  TestPolylineEncode(name + "2", points, maxPoint, &EncodePolylinePrev2, &DecodePolylinePrev2);
  TestPolylineEncode(name + "3", points, maxPoint, &EncodePolylinePrev3, &DecodePolylinePrev3);
}
}  // namespace

UNIT_TEST(EncodePointDeltaAsUint)
{
  for (int x = -100; x <= 100; ++x)
  {
    for (int y = -100; y <= 100; ++y)
    {
      PU orig = PU(100 + x, 100 + y);
      PU pred = PU(100, 100);
      TEST_EQUAL(orig, DecodePointDeltaFromUint(EncodePointDeltaAsUint(orig, pred), pred), ());
      vector<char> data;
      PushBackByteSink<vector<char>> sink(data);
      WriteVarUint(sink, EncodePointDeltaAsUint(orig, pred));
      size_t expectedSize = 1;
      if (x >= 8 || x < -8 || y >= 4 || y < -4)
        expectedSize = 2;
      if (x >= 64 || x < -64 || y >= 64 || y < -64)
        expectedSize = 3;
      TEST_EQUAL(data.size(), expectedSize, (x, y));
    }
  }
}

UNIT_TEST(PredictPointsInPolyline2)
{
  // Ci = Ci-1 + (Ci-1 + Ci-2) / 2
  TEST_EQUAL(PU(5, 5), PredictPointInPolyline(PD(8, 7), PU(4, 4), PU(1, 2)), ());

  // Clamp max
  TEST_EQUAL(PU(4, 4), PredictPointInPolyline(PD(4, 4), PU(4, 4), PU(1, 2)), ());
  TEST_EQUAL(PU(5, 5), PredictPointInPolyline(PD(8, 7), PU(4, 4), PU(1, 2)), ());
  TEST_EQUAL(PU(5, 5), PredictPointInPolyline(PD(5, 5), PU(4, 4), PU(1, 2)), ());

  // Clamp 0
  TEST_EQUAL(PU(4, 0), PredictPointInPolyline(PD(5, 5), PU(4, 1), PU(4, 4)), ());
}

UNIT_TEST(PredictPointsInTriangle)
{
  // Ci = Ci-1 + Ci-2 - Ci-3
  TEST_EQUAL(PU(1, 1), PredictPointInTriangle(PD(100, 100), PU(1, 0), PU(0, 1), PU(0, 0)), ());

  // Clamp 0
  TEST_EQUAL(PU(0, 0), PredictPointInTriangle(PD(100, 100), PU(1, 0), PU(0, 1), PU(5, 5)), ());

  // Clamp max
  TEST_EQUAL(PU(10, 10), PredictPointInTriangle(PD(10, 10), PU(8, 7), PU(6, 5), PU(1, 1)), ());
}

/*
UNIT_TEST(PredictPointsInPolyline3_Square)
{
  TEST_EQUAL(PU(5, 1), PredictPointInPolyline(PU(6, 6), PU(5, 4), PU(2, 4), PU(2, 1)), ());
  TEST_EQUAL(PU(5, 3), PredictPointInPolyline(PU(6, 6), PU(4, 1), PU(2, 2), PU(3, 4)), ());
}

UNIT_TEST(PredictPointsInPolyline3_SquareClamp0)
{
  TEST_EQUAL(PU(5, 1), PredictPointInPolyline(PU(6, 6), PU(5, 4), PU(2, 4), PU(2, 1)), ());
  TEST_EQUAL(PU(4, 0), PredictPointInPolyline(PU(6, 6), PU(2, 0), PU(3, 2), PU(5, 1)), ());
}

UNIT_TEST(PredictPointsInPolyline3_90deg)
{
  TEST_EQUAL(PU(3, 2), PredictPointInPolyline(PU(8, 8), PU(3, 6), PU(1, 6), PU(1, 5)), ());
}
*/

UNIT_TEST(EncodePolyline)
{
  size_t const kSizes[] = {0, 1, 2, 3, 4, ARRAY_SIZE(LargePolygon::kLargePolygon)};
  m2::PointU const maxPoint(1000000000, 1000000000);
  for (size_t iSize = 0; iSize < ARRAY_SIZE(kSizes); ++iSize)
  {
    size_t const polygonSize = kSizes[iSize];
    vector<m2::PointU> points;
    points.reserve(polygonSize);
    for (size_t i = 0; i < polygonSize; ++i)
      points.push_back(m2::PointU(static_cast<uint32_t>(LargePolygon::kLargePolygon[i].x * 10000),
                                  static_cast<uint32_t>((LargePolygon::kLargePolygon[i].y + 200) * 10000)));

    TestEncodePolyline("Unsimp", maxPoint, points);
    TestEncodePolyline("1simp", maxPoint, SimplifyPoints(points, 1));
    TestEncodePolyline("2simp", maxPoint, SimplifyPoints(points, 2));
    TestEncodePolyline("4simp", maxPoint, SimplifyPoints(points, 4));
    TestEncodePolyline("10simp", maxPoint, SimplifyPoints(points, 10));
    TestEncodePolyline("100simp", maxPoint, SimplifyPoints(points, 100));
    TestEncodePolyline("500simp", maxPoint, SimplifyPoints(points, 500));
    TestEncodePolyline("1000simp", maxPoint, SimplifyPoints(points, 1000));
    TestEncodePolyline("2000simp", maxPoint, SimplifyPoints(points, 2000));
    TestEncodePolyline("4000simp", maxPoint, SimplifyPoints(points, 4000));
  }
}

// see 476c1d1d125f0c2deb8c commit for special decode test

UNIT_TEST(DecodeEncodePolyline_DataSet1)
{
  size_t const count = ARRAY_SIZE(geometry_coding_tests::arr1);
  vector<m2::PointU> points;
  points.reserve(count);
  for (size_t i = 0; i < count; ++i)
    points.push_back(D2U(geometry_coding_tests::arr1[i]));

  TestPolylineEncode("DataSet1", points, GetMaxPoint(), &EncodePolyline, &DecodePolyline);
}
