#include "testing/testing.hpp"

#include "indexer/city_boundary.hpp"
#include "indexer/cities_boundaries_serdes.hpp"
#include "indexer/coding_params.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <cstdint>
#include <vector>

using namespace indexer;
using namespace m2;
using namespace serial;
using namespace std;

namespace
{
using Boundary = vector<CityBoundary>;
using Boundaries = vector<Boundary>;

void TestEqual(BoundingBox const & lhs, BoundingBox const & rhs, double eps)
{
  TEST(AlmostEqualAbs(lhs.Min(), rhs.Min(), eps), (lhs, rhs));
  TEST(AlmostEqualAbs(lhs.Max(), rhs.Max(), eps), (lhs, rhs));
}

void TestEqual(CalipersBox const & lhs, CalipersBox const & rhs, double eps) {}

void TestEqual(DiamondBox const & lhs, DiamondBox const & rhs, double eps)
{
  auto const lps = lhs.Points();
  auto const rps = rhs.Points();
  TEST_EQUAL(lps.size(), 4, (lhs));
  TEST_EQUAL(rps.size(), 4, (rhs));
  for (size_t i = 0; i < 4; ++i)
    TEST(AlmostEqualAbs(lps[i], rps[i], eps), (lhs, rhs));
}

void TestEqual(CityBoundary const & lhs, CityBoundary const & rhs, double eps)
{
  TestEqual(lhs.m_bbox, rhs.m_bbox, eps);
  TestEqual(lhs.m_cbox, rhs.m_cbox, eps);
  TestEqual(lhs.m_dbox, rhs.m_dbox, eps);
}

void TestEqual(Boundary const & lhs, Boundary const & rhs, double eps)
{
  TEST_EQUAL(lhs.size(), rhs.size(), (lhs, rhs));
  for (size_t i = 0; i < lhs.size(); ++i)
    TestEqual(lhs[i], rhs[i], eps);
}

void TestEqual(Boundaries const & lhs, Boundaries const & rhs, double eps)
{
  TEST_EQUAL(lhs.size(), rhs.size(), (lhs, rhs));
  for (size_t i = 0; i < lhs.size(); ++i)
    TestEqual(lhs[i], rhs[i], eps);
}

Boundaries EncodeDecode(Boundaries const & boundaries, CodingParams const & params)
{
  vector<uint8_t> buffer;
  {
    MemWriter<decltype(buffer)> sink(buffer);
    CityBoundaryEncoder<decltype(sink)> encoder(sink, params);
    encoder(boundaries);
  }

  {
    Boundaries boundaries;
    MemReader reader(buffer.data(), buffer.size());
    NonOwningReaderSource source(reader);
    CityBoundaryDecoder<decltype(source)> decoder(source, params);
    decoder(boundaries);
    return boundaries;
  }
}

void TestEncodeDecode(Boundaries const & expected, CodingParams const & params, double eps)
{
  Boundaries const actual = EncodeDecode(expected, params);
  TestEqual(expected, actual, eps);
}

UNIT_TEST(CitiesBoundariesSerDes_Smoke)
{
  CodingParams const params(19 /* coordBits */, PointD(MercatorBounds::minX, MercatorBounds::minY));
  double const kEps = 1e-3;

  {
    Boundaries const expected;
    TestEncodeDecode(expected, params, kEps);
  }

  {
    Boundary boundary0;
    boundary0.emplace_back(vector<PointD>{{PointD(0.1234, 5.6789)}});
    boundary0.emplace_back(vector<PointD>{{PointD(3.1415, 2.1828), PointD(2.1828, 3.1415)}});

    Boundary boundary1;
    boundary1.emplace_back(
        vector<PointD>{{PointD(1.000, 1.000), PointD(1.002, 1.000), PointD(1.002, 1.003)}});

    Boundaries const expected = {{boundary0, boundary1}};
    TestEncodeDecode(expected, params, kEps);
  }
}
}  // namespace
