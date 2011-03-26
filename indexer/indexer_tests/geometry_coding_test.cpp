#include "../geometry_coding.hpp"
#include "../../testing/testing.hpp"

#include "../../indexer/mercator.hpp"
#include "../../indexer/point_to_int64.hpp"

#include "../../geometry/geometry_tests/large_polygon.hpp"
#include "../../geometry/distance.hpp"
#include "../../geometry/simplification.hpp"
#include "../../geometry/pointu_to_uint64.hpp"

#include "../../coding/byte_stream.hpp"
#include "../../coding/varint.hpp"
#include "../../coding/writer.hpp"

#include "../../base/logging.hpp"
#include "../../base/array_adapters.hpp"


typedef m2::PointU PU;

UNIT_TEST(EncodeDelta)
{
  for (int x = -100; x <= 100; ++x)
  {
    for (int y = -100; y <= 100; ++y)
    {
      PU orig = PU(100 + x, 100 + y);
      PU pred = PU(100, 100);
      TEST_EQUAL(orig, DecodeDelta(EncodeDelta(orig, pred), pred), ());
      vector<char> data;
      PushBackByteSink<vector<char> > sink(data);
      WriteVarUint(sink, EncodeDelta(orig, pred));
      size_t expectedSize = 1;
      if (x >= 8 || x < -8 || y >= 4 || y < -4) expectedSize = 2;
      if (x >= 64 || x < -64 || y >= 64 || y < -64) expectedSize = 3;
      TEST_EQUAL(data.size(), expectedSize, (x, y));
    }
  }
}

UNIT_TEST(PredictPointsInPolyline2)
{
  // Ci = Ci-1 + (Ci-1 + Ci-2) / 2
  TEST_EQUAL(PU(5, 5), PredictPointInPolyline(PU(8, 7), PU(4, 4), PU(1, 2)), ());
}

UNIT_TEST(PredictPointsInPolyline2_ClampMax)
{
  // Ci = Ci-1 + (Ci-1 + Ci-2) / 2
  TEST_EQUAL(PU(4, 4), PredictPointInPolyline(PU(4, 4), PU(4, 4), PU(1, 2)), ());
  TEST_EQUAL(PU(5, 5), PredictPointInPolyline(PU(8, 7), PU(4, 4), PU(1, 2)), ());
  TEST_EQUAL(PU(5, 5), PredictPointInPolyline(PU(5, 5), PU(4, 4), PU(1, 2)), ());
}

UNIT_TEST(PredictPointsInPolyline2_Clamp0)
{
  TEST_EQUAL(PU(4, 0), PredictPointInPolyline(PU(5, 5), PU(4, 1), PU(4, 4)), ());
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

namespace
{

void TestPolylineEncode(string testName,
                        vector<m2::PointU> const & points,
                        m2::PointU const & maxPoint,
                        void (* fnEncode)(geo_coding::InPointsT const & points,
                                          m2::PointU const & basePoint,
                                          m2::PointU const & maxPoint,
                                          geo_coding::OutDeltasT & deltas),
                        void (* fnDecode)(geo_coding::InDeltasT const & deltas,
                                          m2::PointU const & basePoint,
                                          m2::PointU const & maxPoint,
                                          geo_coding::OutPointsT & points))
{
  size_t const count = points.size();
  if (count == 0) return;

  m2::PointU const basePoint = points[count / 2];

  vector<uint64_t> deltas;
  deltas.resize(count);

  geo_coding::OutDeltasT deltasA(deltas);
  fnEncode(make_read_adapter(points), basePoint, maxPoint, deltasA);

  vector<m2::PointU> decodedPoints;
  decodedPoints.resize(count);

  geo_coding::OutPointsT decodedPointsA(decodedPoints);
  fnDecode(make_read_adapter(deltas), basePoint, maxPoint, decodedPointsA);

  TEST_EQUAL(points, decodedPoints, ());

  if (points.size() > 10)
  {
    vector<char> data;
    MemWriter<vector<char> > writer(data);

    for (size_t i = 0; i != deltas.size(); ++i)
      WriteVarUint(writer, deltas[i]);

    LOG(LINFO, (testName, points.size(), data.size()));
  }
}

vector<m2::PointU> SimplifyPoints(vector<m2::PointU> const & points, double eps)
{
  vector<m2::PointU> simpPoints;
  typedef mn::DistanceToLineSquare<m2::PointD> DistanceF;
  SimplifyNearOptimal<DistanceF>(20, points.begin(), points.end(), eps,
                                 AccumulateSkipSmallTrg<DistanceF, m2::PointU>(simpPoints, eps));
  return simpPoints;
}

void TestEncodePolyline(string name, m2::PointU maxPoint, vector<m2::PointU> const & points)
{
  using namespace geo_coding;

  TestPolylineEncode(name + "1", points, maxPoint, &EncodePolylinePrev1, &DecodePolylinePrev1);
  TestPolylineEncode(name + "2", points, maxPoint, &EncodePolylinePrev2, &DecodePolylinePrev2);
  TestPolylineEncode(name + "3", points, maxPoint, &EncodePolylinePrev3, &DecodePolylinePrev3);
}

}

UNIT_TEST(EncodePolyline)
{
  size_t const kSizes [] = { 0, 1, 2, 3, 4, ARRAY_SIZE(LargePolygon::kLargePolygon) };
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

UNIT_TEST(DecodePolyline_Values)
{
  uint64_t arr[] = {
    2734738402133224327,
    587102334176945,
    154352840547926,
    43024411088417,
    2208572839502425,
    1537729724693572015,
    110442683719211
  };

  int64_t const baseP = 876424843953707746;

  array_read<uint64_t> in(arr, ARRAY_SIZE(arr));

  vector<m2::PointU> buffer(10);
  array_write<m2::PointU> out(buffer);

  geo_coding::DecodePolylinePrev2(in,
                      m2::Uint64ToPointU(baseP), PointD2PointU(MercatorBounds::maxX, MercatorBounds::maxY),
                      out);

  for (size_t i = 0; i < out.size(); ++i)
  {
    CoordPointT const pt = PointU2PointD(out[i]);
    TEST ( MercatorBounds::minX <= pt.first && pt.first <= MercatorBounds::maxX, (pt.first) );
    TEST ( MercatorBounds::minY <= pt.second && pt.second <= MercatorBounds::maxY, (pt.second) );
  }
}
