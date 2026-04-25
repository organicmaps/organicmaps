#include "testing/testing.hpp"
#include "tmp_mwm_copy.hpp"

#include "indexer/data_header.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/features_vector.hpp"

#include "platform/platform.hpp"

#include "coding/files_container.hpp"
#include "coding/mmap_reader.hpp"

#include "defines.hpp"

#include <memory>
#include <string>

namespace features_offsets_table_test
{
using namespace feature;
using namespace platform;
using namespace std;

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
  size_t constexpr minFeaturesCount = 5000;
  tests::TempMwmCopy mwmCopy("minsk-pass");
  auto const & mwmPath = mwmCopy.GetPath();

  FilesContainerR cont(GetPlatform().GetReader("minsk-pass" DATA_FILE_EXTENSION));
  unique_ptr<FeaturesOffsetsTable> srcTable(FeaturesOffsetsTable::Load(cont, FEATURE_OFFSETS_FILE_TAG));
  TEST(srcTable.get() && srcTable->size() > minFeaturesCount, ());

  BuildOffsetsTable(mwmPath);

  FilesContainerR newCont(std::make_unique<MmapReader>(mwmPath));
  unique_ptr<FeaturesOffsetsTable> newTable(FeaturesOffsetsTable::Load(newCont, FEATURE_OFFSETS_FILE_TAG));

  TEST_EQUAL(srcTable->size(), newTable->size(), ());
  for (uint64_t i = 0; i < srcTable->size(); ++i)
    TEST_EQUAL(srcTable->GetFeatureOffset(i), newTable->GetFeatureOffset(i), ());
}

}  // namespace features_offsets_table_test
