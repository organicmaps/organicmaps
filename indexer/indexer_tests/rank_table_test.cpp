#include "testing/testing.hpp"

#include "indexer/rank_table.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"

#include "base/scope_guard.hpp"

#include "std/vector.hpp"

namespace
{
void TestTable(vector<uint8_t> const & ranks, search::RankTable const & table)
{
  TEST_EQUAL(ranks.size(), table.Size(), ());
  TEST_EQUAL(table.GetVersion(), search::RankTable::V1, ());
  for (size_t i = 0; i < ranks.size(); ++i)
    TEST_EQUAL(i, table.Get(i), ());
}
}  // namespace

UNIT_TEST(FeatureRankTableBuilder_Smoke)
{
  char const kTestCont[] = "test.tmp";
  size_t const kNumRanks = 256;

  FileWriter::DeleteFileX(kTestCont);
  MY_SCOPE_GUARD(cleanup, bind(&FileWriter::DeleteFileX, kTestCont));

  vector<uint8_t> ranks;
  for (size_t i = 0; i < kNumRanks; ++i)
    ranks.push_back(i);

  {
    FilesContainerW wcont(kTestCont);
    search::RankTableBuilder::Create(ranks, wcont);
  }

  // Tries to load table via file read.
  {
    FilesContainerR rcont(kTestCont);
    auto table = search::RankTable::Load(rcont);
    TEST(table, ());
    TestTable(ranks, *table);
  }

  // Tries to load table via file mapping.
  {
    FilesMappingContainer mcont(kTestCont);
    auto table = search::RankTable::Load(mcont);
    TEST(table, ());
    TestTable(ranks, *table);
  }
}
