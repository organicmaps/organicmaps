#include "testing/testing.hpp"

#include "indexer/terrain/isolines_tracer.hpp"
#include "indexer/terrain/terrain_reader.hpp"
#include "indexer/terrain/twm_set.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <cmath>

namespace isolines_tracer_tests
{
std::string GetTestBlockPath()
{
  return base::JoinPath(GetPlatform().ResourcesDir(), "N43E007.twm");
}

// Every open contour of the real Nice fixture must end at the block boundary.
UNIT_TEST(TerrainIsolines_RealInvariants)
{
  terrain::Reader const reader(GetTestBlockPath());
  auto const & header = reader.GetHeader();
  auto const rect = header.GetLimitRect();
  size_t const geomIndex = header.m_geometries.size() - 1;
  terrain::TerrainData data(header.m_coordBits);
  reader.ReadTerrain(rect, geomIndex, data);
  TEST(!data.m_mesh.IsEmpty(), ());

  auto const onBoundary = [&](m2::PointD const & point)
  {
    return std::min({std::abs(point.x - rect.minX()), std::abs(point.x - rect.maxX()), std::abs(point.y - rect.minY()),
                     std::abs(point.y - rect.maxY())}) < 1e-6;
  };
  size_t lines = 0, open = 0, closed = 0;
  terrain::TraceIsolines(data, 100, measurement_utils::Units::Metric, [&](terrain::Isoline && isoline)
  {
    ++lines;
    TEST_EQUAL(isoline.m_altitude % 100, 0, ());
    auto const & points = isoline.m_points;
    TEST_GREATER_OR_EQUAL(points.size(), 2, (isoline.m_altitude));
    if (isoline.m_closed)
    {
      ++closed;
      TEST_GREATER_OR_EQUAL(points.size(), 4, (isoline.m_altitude));
      TEST_EQUAL(points.front(), points.back(), (isoline.m_altitude));
    }
    else
    {
      ++open;
      TEST(onBoundary(points.front()), (isoline.m_altitude, points.front()));
      TEST(onBoundary(points.back()), (isoline.m_altitude, points.back()));
    }
    for (auto const & point : points)
    {
      TEST(std::isfinite(point.x) && std::isfinite(point.y), (isoline.m_altitude));
      TEST(rect.IsPointInside(point), (isoline.m_altitude, point));
    }
  });
  TEST_GREATER(lines, 0, ());
  TEST_GREATER(open, 0, ());
  TEST_GREATER(closed, 0, ());

  base::SharedCancellable cancellation;
  cancellation.Cancel();
  terrain::TerrainData cancelled(header.m_coordBits);
  reader.ReadTerrain(rect, geomIndex, cancelled, cancellation);
  TEST(cancelled.IsEmpty(), ());
}

// A steep triangle crossing over a thousand levels is valid data: it must trace in both units.
UNIT_TEST(TerrainIsolines_SteepTriangle)
{
  terrain::TileMesh mesh;
  auto const a = mesh.AddVertex({1000, 1000}, 0);
  auto const b = mesh.AddVertex({3000, 1000}, 7000);
  auto const c = mesh.AddVertex({1000, 3000}, 0);
  mesh.AddTriangle(a, b, c);
  for (auto const units : {measurement_utils::Units::Metric, measurement_utils::Units::Imperial})
  {
    int32_t const step = units == measurement_utils::Units::Metric ? 10 : 20;
    size_t lines = 0;
    terrain::TraceIsolines(mesh, step, units, [&](terrain::Isoline && isoline)
    {
      TEST_EQUAL(isoline.m_altitude % step, 0, ());
      ++lines;
    });
    // 7000 m is 700 levels at 10 m and 1148 levels at 20 ft.
    TEST_GREATER(lines, units == measurement_utils::Units::Metric ? 690 : 1140, (lines));
  }
}

// Registration keeps the scanned file identity and rejects overlapping active blocks.
UNIT_TEST(TerrainTwmSet_Registration)
{
  std::string const source = GetTestBlockPath();
  // Two copies of the same block = the same header rect = a guaranteed overlap.
  std::string const newerPath = base::JoinPath(GetPlatform().TmpDir(), "twm_v2.twm");
  std::string const olderPath = base::JoinPath(GetPlatform().TmpDir(), "twm_v1.twm");
  TEST(base::CopyFileX(source, newerPath), ());
  TEST(base::CopyFileX(source, olderPath), ());
  SCOPE_GUARD(cleanup, [&]()
  {
    base::DeleteFileX(newerPath);
    base::DeleteFileX(olderPath);
  });

  terrain::TwmFile newer, older;
  TEST_EQUAL(terrain::TwmSet::ReadFile(newerPath, 2 /* version */, newer), terrain::TwmSet::RegResult::Success, ());
  TEST_EQUAL(terrain::TwmSet::ReadFile(olderPath, 1 /* version */, older), terrain::TwmSet::RegResult::Success, ());
  TEST(newer.m_rect.IsValid(), ());

  terrain::TwmSet set;
  auto const [id, result] = set.Register(newer);
  TEST_EQUAL(result, terrain::TwmSet::RegResult::Success, ());
  TEST_EQUAL(set.GetId(newer.m_id), id, ());
  TEST_EQUAL(id.GetInfo()->GetFilePath(), newerPath, ());
  TEST_EQUAL(id.GetInfo()->GetVersion(), 2, ());
  TEST_EQUAL(set.Register(older).second, terrain::TwmSet::RegResult::Overlapping, ());
  TEST(set.HasBlocks(newer.m_rect), ());

  set.Deregister(newer.m_id);
  TEST(!set.GetId(newer.m_id).IsAlive(), ());
  TEST_EQUAL(set.Register(older).second, terrain::TwmSet::RegResult::Success, ());
  TEST_EQUAL(set.GetId(older.m_id).GetInfo()->GetVersion(), 1, ());
}
}  // namespace isolines_tracer_tests
