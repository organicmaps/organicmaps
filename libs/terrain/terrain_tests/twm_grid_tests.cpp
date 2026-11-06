#include "testing/testing.hpp"

#include "terrain/twm_grid.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"

#include "base/file_name_utils.hpp"

#include <string>

namespace twm_grid_tests
{
using terrain::GridBlock;

UNIT_TEST(TwmGrid_BlockNameRoundTrip)
{
  struct Case
  {
    int m_left, m_bottom;
    char const * m_name;
  };
  for (auto const & c : {Case{7, 43, "N43E007"}, Case{-75, -14, "S14W075"}, Case{0, 0, "N00E000"},
                         Case{-180, -85, "S85W180"}, Case{179, 84, "N84E179"}, Case{-1, -1, "S01W001"}})
  {
    GridBlock const block{c.m_left, c.m_bottom, 1, 1};
    TEST_EQUAL(block.GetFileName(), std::string(c.m_name) + ".twm", ());
    int bottom = 0, left = 0;
    TEST(terrain::ParseBlockName(c.m_name, bottom, left), (c.m_name));
    TEST_EQUAL(bottom, c.m_bottom, (c.m_name));
    TEST_EQUAL(left, c.m_left, (c.m_name));
  }
}

UNIT_TEST(TwmGrid_ParseBlockNameRejectsMalformed)
{
  int bottom = 0, left = 0;
  for (char const * name : {"", "N43E07", "N43E0070", "X43E007", "N43X007", "n43e007", "N-5E007", "N+5E007", "N43E-07",
                            "N43E 07", "N4aE007", "N43E007.twm", "S00E007", "N43W000"})
    TEST(!terrain::ParseBlockName(name, bottom, left), (name));
}

UNIT_TEST(TwmGrid_IsValidBlock)
{
  TEST(terrain::IsValidBlock({-180, -85, 360, 170}), ());
  TEST(terrain::IsValidBlock({7, 43, 1, 1}), ());
  TEST(!terrain::IsValidBlock({7, 43, 0, 1}), ());
  TEST(!terrain::IsValidBlock({7, 43, 1, 0}), ());
  TEST(!terrain::IsValidBlock({179, 43, 2, 1}), ("Beyond +180"));
  TEST(!terrain::IsValidBlock({-181, 43, 1, 1}), ("Beyond -180"));
  TEST(!terrain::IsValidBlock({7, 84, 1, 2}), ("Beyond the mercator-safe band"));
  TEST(!terrain::IsValidBlock({7, -86, 1, 1}), ("Beyond the mercator-safe band"));
}

UNIT_TEST(TwmGrid_ListVersionDirsNewestFirst)
{
  std::string const terrainDir = base::JoinPath(GetPlatform().WritableDir(), "twm_grid_tests");
  platform::tests_support::ScopedDirCleanup const cleanup(terrainDir);
  for (char const * dir : {"9", "261104", "0", "-3", "latest", "260721"})
    TEST_EQUAL(Platform::MkDir(base::JoinPath(terrainDir, dir)), Platform::ERR_OK, (dir));

  auto const dirs = terrain::ListVersionDirs(terrainDir);
  TEST_EQUAL(dirs.size(), 3, ("Only the positive numeric folders"));
  TEST_EQUAL(dirs[0].m_version, 261104, ());
  TEST_EQUAL(dirs[1].m_version, 260721, ());
  TEST_EQUAL(dirs[2].m_version, 9, ());
  TEST_EQUAL(dirs[0].m_path, base::JoinPath(terrainDir, "261104"), ());
}
}  // namespace twm_grid_tests
