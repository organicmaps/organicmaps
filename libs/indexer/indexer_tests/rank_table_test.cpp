#include "testing/testing.hpp"
#include "tmp_mwm_copy.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"

#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <memory>
#include <string>
#include <vector>

namespace rank_table_test
{
using namespace std;

namespace
{
void TestTable(vector<uint8_t> const & ranks, search::RankTable const & table)
{
  TEST_EQUAL(ranks.size(), table.Size(), ());
  TEST_EQUAL(table.GetVersion(), search::RankTable::V0, ());
  for (size_t i = 0; i < ranks.size(); ++i)
    TEST_EQUAL(ranks[i], table.Get(i), ());
}

void TestTable(vector<uint8_t> const & ranks, string const & path)
{
  // Tries to load table via file read.
  {
    FilesContainerR rcont(path);
    auto table = search::RankTable::Load(rcont, SEARCH_RANKS_FILE_TAG);
    TEST(table, ());
    TestTable(ranks, *table);
  }

  // Tries to load table via file mapping.
  {
    FilesContainerR mcont(std::make_unique<MmapReader>(path));
    auto table = search::RankTable::Load(mcont, SEARCH_RANKS_FILE_TAG);
    TEST(table, ());
    TestTable(ranks, *table);
  }
}
}  // namespace

UNIT_TEST(RankTableBuilder_Smoke)
{
  char const kTestCont[] = "test.tmp";
  size_t const kNumRanks = 256;

  FileWriter::DeleteFileX(kTestCont);
  SCOPE_GUARD(cleanup, bind(&FileWriter::DeleteFileX, kTestCont));

  vector<uint8_t> ranks;
  for (size_t i = 0; i < kNumRanks; ++i)
    ranks.push_back(i);

  {
    FilesContainerW wcont(kTestCont);
    search::RankTableBuilder::Create(ranks, wcont, SEARCH_RANKS_FILE_TAG);
  }

  TestTable(ranks, kTestCont);
}

UNIT_TEST(RankTableBuilder_EndToEnd)
{
  classificator::Load();

  tests::TempMwmCopy mwmCopy("minsk-pass");
  auto const & mwmPath = mwmCopy.GetPath();

  vector<uint8_t> ranks;
  {
    FilesContainerR rcont(mwmPath);
    search::SearchRankTableBuilder::CalcSearchRanks(rcont, ranks);
  }

  {
    FilesContainerW wcont(mwmPath, FileWriter::OP_WRITE_EXISTING);
    search::RankTableBuilder::Create(ranks, wcont, SEARCH_RANKS_FILE_TAG);
  }

  FrozenDataSource dataSource;
  auto regResult = dataSource.RegisterMap(mwmCopy.GetLocalFile());
  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());

  TestTable(ranks, mwmPath);
}
}  // namespace rank_table_test
