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

static_assert(CitiesBoundariesSerDes::kLatestVersion == 0, "");
static_assert(CitiesBoundariesSerDes::HeaderV0::kDefaultCoordBits == 19, "");

// Absolute precision of mercator coords encoded with 19 bits.
double const kEps = 1e-3;

void TestEqual(vector<PointD> const & lhs, vector<PointD> const & rhs)
{
  TEST_EQUAL(lhs.size(), rhs.size(), (lhs, rhs));
  for (size_t i = 0; i < lhs.size(); ++i)
    TEST(AlmostEqualAbs(lhs[i], rhs[i], kEps), (lhs, rhs));
}

void TestEqual(BoundingBox const & lhs, BoundingBox const & rhs)
{
  TEST(AlmostEqualAbs(lhs.Min(), rhs.Min(), kEps), (lhs, rhs));
  TEST(AlmostEqualAbs(lhs.Max(), rhs.Max(), kEps), (lhs, rhs));
}

void TestEqual(CalipersBox const & lhs, CalipersBox const & rhs)
{
  TestEqual(lhs.Points(), rhs.Points());
}

void TestEqual(DiamondBox const & lhs, DiamondBox const & rhs)
{
  auto const lps = lhs.Points();
  auto const rps = rhs.Points();
  TEST_EQUAL(lps.size(), 4, (lhs));
  TEST_EQUAL(rps.size(), 4, (rhs));
  TestEqual(lps, rps);
}

void TestEqual(CityBoundary const & lhs, CityBoundary const & rhs)
{
  TestEqual(lhs.m_bbox, rhs.m_bbox);
  TestEqual(lhs.m_cbox, rhs.m_cbox);
  TestEqual(lhs.m_dbox, rhs.m_dbox);
}

void TestEqual(Boundary const & lhs, Boundary const & rhs)
{
  TEST_EQUAL(lhs.size(), rhs.size(), (lhs, rhs));
  for (size_t i = 0; i < lhs.size(); ++i)
    TestEqual(lhs[i], rhs[i]);
}

void TestEqual(Boundaries const & lhs, Boundaries const & rhs)
{
  TEST_EQUAL(lhs.size(), rhs.size(), (lhs, rhs));
  for (size_t i = 0; i < lhs.size(); ++i)
    TestEqual(lhs[i], rhs[i]);
}

Boundaries EncodeDecode(Boundaries const & boundaries)
{
  vector<uint8_t> buffer;
  {
    MemWriter<decltype(buffer)> sink(buffer);
    CitiesBoundariesSerDes::Serialize(sink, boundaries);
  }

  {
    Boundaries boundaries;
    MemReader reader(buffer.data(), buffer.size());
    NonOwningReaderSource source(reader);
    CitiesBoundariesSerDes::Deserialize(source, boundaries);
    return boundaries;
  }
}

void TestEncodeDecode(Boundaries const & expected)
{
  Boundaries const actual = EncodeDecode(expected);
  TestEqual(expected, actual);
}

UNIT_TEST(CitiesBoundariesSerDes_Smoke)
{
  {
    Boundaries const expected;
    TestEncodeDecode(expected);
  }

  {
    Boundary boundary0;
    boundary0.emplace_back(vector<PointD>{{PointD(0.1234, 5.6789)}});
    boundary0.emplace_back(vector<PointD>{{PointD(3.1415, 2.1828), PointD(2.1828, 3.1415)}});

    Boundary boundary1;
    boundary1.emplace_back(
        vector<PointD>{{PointD(1.000, 1.000), PointD(1.002, 1.000), PointD(1.002, 1.003)}});

    Boundaries const expected = {{boundary0, boundary1}};
    TestEncodeDecode(expected);
  }
}
}  // namespace
