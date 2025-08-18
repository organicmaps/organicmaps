#include "testing/testing.hpp"

#include "indexer/data_header.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/features_vector.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/files_container.hpp"

#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <functional>
#include <memory>
#include <string>

using namespace platform;
using namespace std;

namespace feature
{
UNIT_TEST(FeaturesOffsetsTable_Empty)
{
  FeaturesOffsetsTable::Builder builder;
  unique_ptr<FeaturesOffsetsTable> table(FeaturesOffsetsTable::Build(builder));
  TEST(table.get(), ());
  TEST_EQUAL(static_cast<uint64_t>(0), table->size(), ());
}

UNIT_TEST(FeaturesOffsetsTable_Basic)
{
  FeaturesOffsetsTable::Builder builder;
  builder.PushOffset(1);
  builder.PushOffset(4);
  builder.PushOffset(17);
  builder.PushOffset(128);
  builder.PushOffset(129);
  builder.PushOffset(510);
  builder.PushOffset(513);
  builder.PushOffset(1024);

  unique_ptr<FeaturesOffsetsTable> table(FeaturesOffsetsTable::Build(builder));
  TEST(table.get(), ());
  TEST_EQUAL(static_cast<uint64_t>(8), table->size(), ());

  TEST_EQUAL(static_cast<uint64_t>(1), table->GetFeatureOffset(0), ());
  TEST_EQUAL(static_cast<uint64_t>(4), table->GetFeatureOffset(1), ());
  TEST_EQUAL(static_cast<uint64_t>(17), table->GetFeatureOffset(2), ());
  TEST_EQUAL(static_cast<uint64_t>(128), table->GetFeatureOffset(3), ());
  TEST_EQUAL(static_cast<uint64_t>(129), table->GetFeatureOffset(4), ());
  TEST_EQUAL(static_cast<uint64_t>(510), table->GetFeatureOffset(5), ());
  TEST_EQUAL(static_cast<uint64_t>(513), table->GetFeatureOffset(6), ());
  TEST_EQUAL(static_cast<uint64_t>(1024), table->GetFeatureOffset(7), ());

  TEST_EQUAL(static_cast<size_t>(0), table->GetFeatureIndexbyOffset(1), ());
  TEST_EQUAL(static_cast<size_t>(1), table->GetFeatureIndexbyOffset(4), ());
  TEST_EQUAL(static_cast<size_t>(2), table->GetFeatureIndexbyOffset(17), ());
  TEST_EQUAL(static_cast<size_t>(3), table->GetFeatureIndexbyOffset(128), ());
  TEST_EQUAL(static_cast<size_t>(4), table->GetFeatureIndexbyOffset(129), ());
  TEST_EQUAL(static_cast<size_t>(5), table->GetFeatureIndexbyOffset(510), ());
  TEST_EQUAL(static_cast<size_t>(6), table->GetFeatureIndexbyOffset(513), ());
  TEST_EQUAL(static_cast<size_t>(7), table->GetFeatureIndexbyOffset(1024), ());
}

UNIT_TEST(FeaturesOffsetsTable_ReadWrite)
{
  string const testFileName = "test_file";
  Platform & pl = GetPlatform();

  FilesContainerR baseContainer(pl.GetReader("minsk-pass" DATA_FILE_EXTENSION));
  size_t constexpr minFeaturesCount = 5000;

  LocalCountryFile localFile = LocalCountryFile::MakeForTesting(testFileName);
  CountryIndexes::PreparePlaceOnDisk(localFile);

  string const indexFile = CountryIndexes::GetPath(localFile, CountryIndexes::Index::Offsets);
  SCOPE_GUARD(deleteTestFileIndexGuard, bind(&FileWriter::DeleteFileX, cref(indexFile)));

  FeaturesOffsetsTable::Build(baseContainer, indexFile);

  unique_ptr<FeaturesOffsetsTable> table(FeaturesOffsetsTable::Load(baseContainer, FEATURE_OFFSETS_FILE_TAG));
  TEST(table.get() && table->size() > minFeaturesCount, ());

  unique_ptr<FeaturesOffsetsTable> loadedTable(FeaturesOffsetsTable::Load(indexFile));
  TEST(loadedTable.get() && loadedTable->size() > minFeaturesCount, ());

  TEST_EQUAL(table->size(), loadedTable->size(), ());
  for (uint64_t i = 0; i < table->size(); ++i)
    TEST_EQUAL(table->GetFeatureOffset(i), loadedTable->GetFeatureOffset(i), ());
}
}  // namespace feature
