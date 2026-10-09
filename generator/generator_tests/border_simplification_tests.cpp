#include "testing/testing.hpp"

#include "generator/border_simplification.hpp"
#include "generator/borders.hpp"

#include "platform/platform.hpp"

#include "indexer/scales.hpp"

#include "coding/files_container.hpp"
#include "coding/read_write_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include "storage/country_decl.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace border_simplification_tests
{
using Edge = std::pair<m2::PointD, m2::PointD>;

m2::RegionD MakeRing(std::vector<m2::PointD> points)
{
  return m2::RegionD(std::move(points));
}

std::set<Edge> Edges(m2::RegionD const & ring)
{
  std::set<Edge> edges;
  auto const & points = ring.Data();
  for (size_t i = 0; i < points.size(); ++i)
  {
    auto const & first = points[i];
    auto const & last = points[(i + 1) % points.size()];
    if (first != last)
      edges.emplace(std::min(first, last), std::max(first, last));
  }
  return edges;
}

std::set<Edge> SharedEdges(m2::RegionD const & first, m2::RegionD const & second)
{
  auto const a = Edges(first);
  auto const b = Edges(second);
  std::set<Edge> shared;
  std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(shared, shared.end()));
  return shared;
}

UNIT_TEST(PackedBorders_RotatedSharedArc)
{
  std::vector<m2::RegionD> const rings = {MakeRing({{0, 0}, {1, 0}, {1.0005, 1}, {1, 2}, {0, 2}, {0, 0}}),
                                          MakeRing({{1.0005, 1}, {1, 0}, {2, 0}, {2, 2}, {1, 2}, {1.0005, 1}})};
  auto const simplified = borders::SimplifyBorders(rings, math::Pow2(scales::GetEpsilonForSimplify(10)));
  std::set<Edge> const expected = {{{1, 0}, {1, 2}}};
  TEST_EQUAL(SharedEdges(simplified[0], simplified[1]), expected, ());
  TEST(simplified[0].Contains({1.000125, 1.5}) || simplified[1].Contains({1.000125, 1.5}), ());
}

UNIT_TEST(PackedBorders_TripleJunction)
{
  std::vector<m2::RegionD> const rings = {
      MakeRing({{0, 0}, {1, 0}, {1.001, .5}, {1, 1}, {1.001, 1.5}, {1, 2}, {0, 2}, {0, 0}}),
      MakeRing({{1, 1}, {2, 1}, {2, 2}, {1, 2}, {1.001, 1.5}, {1, 1}}),
      MakeRing({{1, 0}, {2, 0}, {2, 1}, {1, 1}, {1.001, .5}, {1, 0}})};
  auto const simplified = borders::SimplifyBorders(rings, .005 * .005);
  for (auto const & ring : simplified)
    TEST(std::find(ring.Begin(), ring.End(), m2::PointD(1, 1)) != ring.End(), ());
  std::set<Edge> const upper = {{{1, 1}, {1, 2}}};
  std::set<Edge> const lower = {{{1, 0}, {1, 1}}};
  TEST_EQUAL(SharedEdges(simplified[0], simplified[1]), upper, ());
  TEST_EQUAL(SharedEdges(simplified[0], simplified[2]), lower, ());
}

UNIT_TEST(PackedBorders_UnsharedOutlinesStillSimplify)
{
  std::vector<m2::RegionD> const rings = {
      MakeRing({{0, 0}, {.5, 0}, {1, 0}, {1.02, .5}, {1, 1}, {.5, 1}, {0, 1}, {0, .5}, {0, 0}}),
      MakeRing({{1, 0}, {1.5, 0}, {2, 0}, {2, .5}, {2, 1}, {1.5, 1}, {1, 1}, {1, 0}})};
  auto const simplified = borders::SimplifyBorders(rings, .005 * .005);
  for (size_t i = 0; i < rings.size(); ++i)
    TEST_LESS(simplified[i].Size(), rings[i].Size(), ());
  TEST(std::find(simplified[0].Begin(), simplified[0].End(), m2::PointD(1.02, .5)) != simplified[0].End(), ());
}

UNIT_TEST(PackedBorders_ClosedRingRotationAndDirection)
{
  std::vector<m2::RegionD> const rings = {
      MakeRing({{0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}, {0, 1}, {0, 0}}),
      MakeRing({{2, 1}, {2, 0}, {1, 0}, {0, 0}, {0, 1}, {0, 2}, {1, 2}, {2, 2}, {2, 1}})};
  auto const simplified = borders::SimplifyBorders(rings, .01 * .01);
  TEST_EQUAL(Edges(simplified[0]), Edges(simplified[1]), ());
  TEST_EQUAL(Edges(simplified[0]).size(), 4, ());
}

UNIT_TEST(PackedBorders_CollapsedRingRollback)
{
  std::vector<m2::RegionD> const rings = {MakeRing({{0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}}),
                                          MakeRing({{0, 0}, {0, -1}, {2, -1}, {2, 2}, {1, 1}, {1, 0}, {0, 0}}),
                                          MakeRing({{0, 0}, {0, 1}, {1, 1}, {1, 2}, {-1, 2}, {-1, -1}, {0, 0}})};
  auto const simplified = borders::SimplifyBorders(rings, 100);
  for (size_t i = 0; i < rings.size(); ++i)
    TEST_EQUAL(simplified[i].Data(), rings[i].Data(), (i));
}

UNIT_TEST(PackedBorders_RepeatedVerticesStayFixed)
{
  std::vector<m2::PointD> const points = {{0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}, {0, 1}};
  auto twice = points;
  twice.insert(twice.end(), points.begin(), points.end());
  twice.push_back(points.front());
  std::vector<m2::RegionD> const rings = {m2::RegionD(twice), m2::RegionD(points)};
  auto const simplified = borders::SimplifyBorders(rings, .01 * .01);
  TEST_EQUAL(simplified[0].Data(), rings[0].Data(), ());
  TEST_EQUAL(simplified[1].Data(), rings[1].Data(), ());
}

UNIT_TEST(PackedBorders_NewIntersectionRollsBack)
{
  std::vector<m2::PointD> const points = {{.117, .027},  {.378, .100},   {.066, .022},   {.643, .281},
                                          {.171, .130},  {.008, .309},   {-.153, .370},  {-.386, .173},
                                          {-.011, .003}, {-.121, -.010}, {-.860, -.087}, {.011, -.003}};
  std::vector<m2::RegionD> const rings = {m2::RegionD(points),
                                          m2::RegionD(std::vector<m2::PointD>(points.rbegin(), points.rend()))};
  auto const simplified = borders::SimplifyBorders(rings, .2 * .2);
  TEST_EQUAL(simplified[0].Data(), rings[0].Data(), ());
  TEST_EQUAL(simplified[1].Data(), rings[1].Data(), ());
}

UNIT_TEST(PackedBorders_SharedArcSerialization)
{
  auto const directory = GetPlatform().TmpPathForFile();
  auto const source = base::JoinPath(directory, BORDERS_DIR);
  TEST(Platform::MkDirRecursively(source), (source));
  SCOPE_GUARD(cleanup, [&] { Platform::RmDirRecursively(directory); });
  std::vector<m2::RegionD> const rings = {MakeRing({{0, 0}, {1, 0}, {1.0005, 1}, {1, 2}, {0, 2}, {0, 0}}),
                                          MakeRing({{1.0005, 1}, {1, 0}, {2, 0}, {2, 2}, {1, 2}, {1.0005, 1}})};
  borders::DumpBorderToPolyFile(source, "A", {rings[0]});
  borders::DumpBorderToPolyFile(source, "B", {rings[1]});
  borders::GeneratePackedBorders(directory + "/");

  std::map<std::string, m2::RegionD> restored;
  FilesContainerR reader(base::JoinPath(directory, PACKED_POLYGONS_FILE));
  std::vector<storage::CountryDef> countries;
  ReaderSource<ModelReaderPtr> info(reader.GetReader(PACKED_POLYGONS_INFO_TAG));
  rw::Read(info, countries);
  TEST_EQUAL(countries.size(), 2, ());
  for (size_t i = 0; i < countries.size(); ++i)
  {
    ReaderSource<ModelReaderPtr> source(reader.GetReader(std::to_string(i)));
    auto polygons = borders::ReadPolygonsOfOneBorder(source);
    TEST_EQUAL(polygons.size(), 1, ());
    restored.emplace(countries[i].m_countryId, std::move(polygons.front()));
  }
  TEST(!SharedEdges(restored.at("A"), restored.at("B")).empty(), ());
  TEST(restored.at("A").Contains({1.000125, 1.5}) || restored.at("B").Contains({1.000125, 1.5}), ());
}
}  // namespace border_simplification_tests
