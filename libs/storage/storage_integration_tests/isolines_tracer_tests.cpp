#include "testing/testing.hpp"

#include "storage/storage.hpp"

#include "indexer/terrain/isolines_tracer.hpp"
#include "indexer/terrain/terrain_reader.hpp"
#include "indexer/terrain/twm_set.hpp"

#include "platform/downloader_utils.hpp"
#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/files_container.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace isolines_tracer_tests
{
// The smallest real planet block (S35W065, the flat Argentinian pampa) of the grid version
// this build ships, downloaded once into the tmp dir. The block must be in the grid and the
// download must succeed: an integration test has no silent offline mode.
std::string GetTestBlockPath()
{
  int64_t version = 0;
  std::vector<storage::Storage::TerrainBlock> blocks;
  std::map<storage::CountryId, std::vector<uint32_t>> coverage;
  std::string content;
  GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
  storage::Storage::ParseTwmGridJson(content, version, blocks, coverage);

  std::string const name = "S35W065";
  auto const it =
      std::find_if(blocks.begin(), blocks.end(), [&name](auto const & block) { return block.m_name == name; });
  TEST(it != blocks.end(), (name));

  std::string const fileName = name + TERRAIN_FILE_EXT;
  std::string const path =
      base::JoinPath(GetPlatform().TmpDir(), "om_test_" + strings::to_string(it->m_version) + "_" + fileName);
  if (!GetPlatform().IsFileExistsByFullPath(path))
  {
    platform::HttpClient client("https://cdn.organicmaps.app/" +
                                downloader::GetTerrainDownloadUrl(it->m_version, fileName));
    client.SetReceivedFile(path);
    bool const downloaded = client.RunHttpRequest() && client.ErrorCode() == 200;
    if (!downloaded)
      base::DeleteFileX(path);
    TEST(downloaded, ("Can't download the test terrain block, error:", client.ErrorCode()));
  }
  uint64_t size = 0;
  TEST(Platform::GetFileSizeByFullPath(path, size), (path));
  TEST_EQUAL(size, it->m_size, (path));
  return path;
}

// The structural isoline invariants on a real flat-plain block: the near-level plains
// exercise the exact-on-level vertices of the half-open classification.
UNIT_TEST(TerrainIsolines_FlatPlainInvariants)
{
  std::string const path = GetTestBlockPath();

  FilesContainerR const container(path);
  terrain::Reader const reader(container);

  m2::RectD const rect =
      mercator::RectByCenterXYAndSizeInMeters(reader.GetHeader().GetLimitRect().Center(), 3000.0 /* meters */);

  size_t const geomIndex = reader.GetHeader().GetGeometryIndex(17 /* the best LOD */);

  // The mesh around the block center decodes and stays in a plausible altitude range.
  terrain::TileMesh mesh(reader.GetHeader().m_coordBits);
  reader.ReadMesh(rect, geomIndex, mesh);
  TEST(!mesh.IsEmpty(), ());
  int32_t minAlt = 20000, maxAlt = -20000;
  for (auto const alt : mesh.GetAltitudes())
  {
    minAlt = std::min(minAlt, alt);
    maxAlt = std::max(maxAlt, alt);
  }
  TEST_GREATER(minAlt, -11000, ());
  TEST_LESS(maxAlt, 9000, ());
  LOG(LINFO, ("Vertices:", mesh.GetPoints().size(), "triangles:", mesh.GetTrianglesCount(), "altitudes:", minAlt, "..",
              maxAlt));

  // The chains are maximal: they end on the COLLECTED mesh boundary, i.e. the union of
  // the intersecting features, which can reach far beyond the query rect.
  m2::RectD meshRect = rect;
  for (auto const & p : mesh.GetPoints())
    meshRect.Add(p);

  // The traced isolines hold the structural invariants even on the degenerate level sets.
  size_t lines = 0;
  std::map<int32_t, size_t> perAltitude;
  terrain::TraceIsolines(mesh, 10, measurement_utils::Units::Metric, [&](terrain::Isoline && isoline)
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
  });
  TEST_GREATER(lines, 0, ());
  for (auto const & [altitude, count] : perAltitude)
    LOG(LINFO, ("Altitude", altitude, ":", count, "isolines"));
}

// The multi-version registry policy: the newer block wins the overlap, the older one is
// rejected; the out-of-date query sees the registered older blocks only.
UNIT_TEST(TerrainTwmSet_VersionPolicy)
{
  std::string const source = GetTestBlockPath();

  m2::RectD rect;
  TEST(terrain::TwmSet::ReadLimitRect(source, rect), ());
  TEST(rect.IsValid(), ());

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

  terrain::TwmSet set;
  TEST_EQUAL(set.Register(newerPath, 2 /* version */).second, terrain::TwmSet::RegResult::Success, ());
  // The older overlapping block is rejected: the newest data wins.
  TEST_EQUAL(set.Register(olderPath, 1 /* version */).second, terrain::TwmSet::RegResult::Overlapping, ());

  TEST(set.HasBlocks(rect), ());
  // The registered v2 block is older than a v3 grid, but not older than itself.
  TEST(set.HasOlderBlocks(rect, 3 /* version */), ());
  TEST(!set.HasOlderBlocks(rect, 2 /* version */), ());
}
}  // namespace isolines_tracer_tests
