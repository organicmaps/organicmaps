#include "testing/testing.hpp"

#include "indexer/terrain/isolines_tracer.hpp"
#include "indexer/terrain/terrain_reader.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/files_container.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>

namespace isolines_tracer_tests
{
// Downloads (once, cached in the tmp dir) the smallest real planet block for the tests:
// S35W065 of the 260729 build, ~14 MB of the flat Argentinian pampa. Returns an empty
// path when the download fails (offline) - the tests then log and pass, but with the
// network they always run on the real data.
std::string GetTestBlockPath()
{
  std::string const path = base::JoinPath(GetPlatform().TmpDir(), "om_test_S35W065.twm");
  if (GetPlatform().IsFileExistsByFullPath(path))
    return path;

  platform::HttpClient client("https://cdn.organicmaps.app/terrain/260729/S35W065.twm");
  client.SetReceivedFile(path);
  if (!client.RunHttpRequest() || client.ErrorCode() != 200)
  {
    LOG(LWARNING, ("Can't download the test terrain block, error:", client.ErrorCode()));
    base::DeleteFileX(path);
    return {};
  }
  return path;
}

// The structural isoline invariants on a real flat-plain block: the near-level plains
// exercise the exact-on-level vertices of the half-open classification.
UNIT_TEST(TerrainIsolines_FlatPlainInvariants)
{
  std::string const path = GetTestBlockPath();
  if (path.empty())
    return;

  FilesContainerR const container(path);
  terrain::Reader const reader(container);
  std::vector<terrain::Reader const *> const readers = {&reader};

  m2::RectD const rect =
      mercator::RectByCenterXYAndSizeInMeters(reader.GetHeader().GetLimitRect().Center(), 3000.0 /* meters */);

  size_t const geomIndex = reader.GetHeader().GetGeometryIndex(17 /* the best LOD */);

  // The triangles around the block center decode and stay in a plausible altitude range.
  size_t features = 0, triangles = 0;
  int32_t minAlt = 20000, maxAlt = -20000;
  // The chains are maximal: they end on the COLLECTED mesh boundary, i.e. the union of
  // the intersecting features, which can reach far beyond the query rect.
  m2::RectD meshRect = rect;
  reader.ForEachFeature(rect, geomIndex, [&](terrain::Triangles const & t)
  {
    ++features;
    triangles += t.m_triangles.size() / 3;
    meshRect.Add(t.m_rect);
    for (auto const alt : t.m_altitudes)
    {
      minAlt = std::min(minAlt, int32_t{alt});
      maxAlt = std::max(maxAlt, int32_t{alt});
    }
  });
  TEST_GREATER(features, 0, ());
  TEST_GREATER(triangles, 0, ());
  TEST_GREATER(minAlt, -11000, ());
  TEST_LESS(maxAlt, 9000, ());
  LOG(LINFO, ("Features:", features, "triangles:", triangles, "altitudes:", minAlt, "..", maxAlt));

  // The traced isolines hold the structural invariants even on the degenerate level sets.
  terrain::IsolinesTracer const tracer(readers);
  size_t lines = 0;
  std::map<int32_t, size_t> perAltitude;
  std::ofstream dump(base::JoinPath(GetPlatform().TmpDir(), "isolines_dump.txt"));
  dump << std::setprecision(12);
  tracer.Trace(rect, geomIndex, 10, measurement_utils::Units::Metric, [&](terrain::Isoline && isoline)
  {
    ++lines;
    ++perAltitude[isoline.m_altitude];
    TEST_EQUAL(isoline.m_altitude % 10, 0, ());
    auto const & pts = isoline.m_points;
    TEST_GREATER_OR_EQUAL(pts.size(), 2, (isoline.m_altitude));
    if (isoline.m_closed)
    {
      TEST_GREATER_OR_EQUAL(pts.size(), 4, (isoline.m_altitude));
      TEST_EQUAL(pts.front(), pts.back(), (isoline.m_altitude));
    }
    for (auto const & p : pts)
    {
      TEST(std::isfinite(p.x) && std::isfinite(p.y), (isoline.m_altitude));
      // A crossing point must not stray outside the collected mesh area.
      TEST(m2::RectD(meshRect.LeftBottom() - m2::PointD(1, 1), meshRect.RightTop() + m2::PointD(1, 1)).IsPointInside(p),
           (isoline.m_altitude, p));
    }
    dump << isoline.m_altitude << ' ' << (isoline.m_closed ? 1 : 0);
    for (auto const & p : pts)
      dump << ' ' << p.x << ',' << p.y;
    dump << '\n';
  });
  TEST_GREATER(lines, 0, ());
  for (auto const & [altitude, count] : perAltitude)
    LOG(LINFO, ("Altitude", altitude, ":", count, "isolines"));
}
}  // namespace isolines_tracer_tests
