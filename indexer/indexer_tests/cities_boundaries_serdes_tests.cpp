#include "testing/testing.hpp"

#include "indexer/cities_boundaries_serdes.hpp"
#include "indexer/city_boundary.hpp"
#include "indexer/coding_params.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <cstdint>
#include <utility>
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

struct Result
{
  Result(Boundaries const & boundaries, double eps) : m_boundaries(boundaries), m_eps(eps) {}

  Boundaries m_boundaries;
  double m_eps = 0.0;
};

void TestEqual(vector<PointD> const & lhs, vector<PointD> const & rhs, double eps)
{
  TEST_EQUAL(lhs.size(), rhs.size(), (lhs, rhs));
  for (size_t i = 0; i < lhs.size(); ++i)
    TEST(AlmostEqualAbs(lhs[i], rhs[i], eps), (lhs, rhs));
}

void TestEqual(BoundingBox const & lhs, BoundingBox const & rhs, double eps)
{
  TEST(AlmostEqualAbs(lhs.Min(), rhs.Min(), eps), (lhs, rhs));
  TEST(AlmostEqualAbs(lhs.Max(), rhs.Max(), eps), (lhs, rhs));
}

void TestEqual(CalipersBox const & lhs, CalipersBox const & rhs, double eps)
{
  TestEqual(lhs.Points(), rhs.Points(), eps);
}

void TestEqual(DiamondBox const & lhs, DiamondBox const & rhs, double eps)
{
  auto const lps = lhs.Points();
  auto const rps = rhs.Points();
  TEST_EQUAL(lps.size(), 4, (lhs));
  TEST_EQUAL(rps.size(), 4, (rhs));
  TestEqual(lps, rps, eps);
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

Result EncodeDecode(Boundaries const & boundaries)
{
  vector<uint8_t> buffer;
  {
    MemWriter<decltype(buffer)> sink(buffer);
    CitiesBoundariesSerDes::Serialize(sink, boundaries);
  }

  {
    Boundaries boundaries;
    double precision;
    MemReader reader(buffer.data(), buffer.size());
    NonOwningReaderSource source(reader);
    CitiesBoundariesSerDes::Deserialize(source, boundaries, precision);
    return Result(boundaries, precision);
  }
}

void TestEncodeDecode(Boundaries const & expected)
{
  auto const r = EncodeDecode(expected);
  TestEqual(expected, r.m_boundaries, r.m_eps);
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

UNIT_TEST(CitiesBoundaries_Moscow)
{
  vector<m2::PointD> const points = {{37.04001, 67.55964},
                                     {37.55650, 66.96428},
                                     {38.02513, 67.37082},
                                     {37.50865, 67.96618}};

  m2::PointD const target(37.44765, 67.65243);

  vector<uint8_t> buffer;
  {
    CityBoundary boundary(points);
    TEST(boundary.HasPoint(target), ());

    MemWriter<decltype(buffer)> sink(buffer);
    CitiesBoundariesSerDes::Serialize(sink, {{boundary}});
  }

  {
    Boundaries boundaries;
    double precision;

    MemReader reader(buffer.data(), buffer.size());
    NonOwningReaderSource source(reader);
    CitiesBoundariesSerDes::Deserialize(source, boundaries, precision);

    TEST_EQUAL(boundaries.size(), 1, ());
    TEST_EQUAL(boundaries[0].size(), 1, ());
    TEST(boundaries[0][0].HasPoint(target, precision), ());
  }
}
}  // namespace
