#include "testing/testing.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/writer.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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
    FilesMappingContainer mcont(path);
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

  string const originalMapPath = base::JoinPath(GetPlatform().WritableDir(), "minsk-pass.mwm");
  string const mapPath = base::JoinPath(GetPlatform().WritableDir(), "minsk-pass-copy.mwm");
  base::CopyFileX(originalMapPath, mapPath);
  SCOPE_GUARD(cleanup, bind(&FileWriter::DeleteFileX, mapPath));

  auto const localFile = platform::LocalCountryFile::MakeForTesting("minsk-pass-copy");
  TEST(localFile.OnDisk(MapFileType::Map), ());

  vector<uint8_t> ranks;
  {
    FilesContainerR rcont(mapPath);
    search::SearchRankTableBuilder::CalcSearchRanks(rcont, ranks);
  }

  {
    FilesContainerW wcont(mapPath, FileWriter::OP_WRITE_EXISTING);
    search::RankTableBuilder::Create(ranks, wcont, SEARCH_RANKS_FILE_TAG);
  }

  FrozenDataSource dataSource;
  auto regResult = dataSource.RegisterMap(localFile);
  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());

  TestTable(ranks, mapPath);
}
