#include "testing/testing.hpp"

#include "indexer/cities_boundaries_serdes.hpp"
#include "indexer/city_boundary.hpp"

#include "platform/platform.hpp"

#include "coding/files_container.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/point2d.hpp"

#include "base/file_name_utils.hpp"

#include <vector>

namespace cities_boundaries_serdes_tests
{
using namespace indexer;
using namespace m2;
using namespace std;

using Boundary = vector<CityBoundary>;
using Boundaries = vector<Boundary>;

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
  // SerDes for CaliperBox works without normalization.
  CalipersBox norm(rhs);
  norm.Normalize();

  TestEqual(lhs.Points(), norm.Points(), eps);
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

// Eps equal function to compare initial (expected) value (lhs) with encoded-decoded (rhs).
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
    boundary1.emplace_back(vector<PointD>{{PointD(1.000, 1.000), PointD(1.002, 1.000), PointD(1.002, 1.003)}});

    Boundaries const expected = {{boundary0, boundary1}};
    TestEncodeDecode(expected);
  }
}

UNIT_TEST(CitiesBoundaries_Moscow)
{
  vector<PointD> const points = {
      {37.04001, 67.55964}, {37.55650, 66.96428}, {38.02513, 67.37082}, {37.50865, 67.96618}};

  PointD const target(37.44765, 67.65243);

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

UNIT_TEST(CitiesBoundaries_Compression)
{
  FilesContainerR cont(base::JoinPath(GetPlatform().WritableDir(), WORLD_FILE_NAME) + DATA_FILE_EXTENSION);

  vector<vector<CityBoundary>> all1;
  double precision;

  ReaderSource source1(cont.GetReader(CITIES_BOUNDARIES_FILE_TAG));
  LOG(LINFO, ("Size before:", source1.Size()));

  CitiesBoundariesSerDes::Deserialize(source1, all1, precision);

  vector<uint8_t> buffer;
  MemWriter writer(buffer);
  CitiesBoundariesSerDes::Serialize(writer, all1);
  LOG(LINFO, ("Size after:", buffer.size()));

  // Equal test.
  vector<vector<CityBoundary>> all2;
  MemReader reader(buffer.data(), buffer.size());
  ReaderSource source2(reader);
  CitiesBoundariesSerDes::Deserialize(source2, all2, precision);

  TEST_EQUAL(all1.size(), all2.size(), ());
  for (size_t i = 0; i < all1.size(); ++i)
  {
    TEST_EQUAL(all1[i].size(), all2[i].size(), ());
    for (size_t j = 0; j < all1[i].size(); ++j)
      if (!(all1[i][j] == all2[i][j]))
        LOG(LINFO, (i, all1[i][j], all2[i][j]));
  }
}

}  // namespace cities_boundaries_serdes_tests
