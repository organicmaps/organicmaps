#include "testing/testing.hpp"

#include "indexer/features_offsets_table.hpp"
#include "indexer/data_header.hpp"
#include "indexer/features_vector.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/file_container.hpp"

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

  UNIT_TEST(FeaturesOffsetsTable_CreateIfNotExistsAndLoad)
  {
    string const testFileName = "minsk-pass";

    LocalCountryFile localFile = LocalCountryFile::MakeForTesting(testFileName);
    string const indexFile = CountryIndexes::GetPath(localFile, CountryIndexes::Index::Offsets);
    FileWriter::DeleteFileX(indexFile);

    unique_ptr<FeaturesOffsetsTable> table = FeaturesOffsetsTable::CreateIfNotExistsAndLoad(localFile);
    MY_SCOPE_GUARD(deleteTestFileIndexGuard, bind(&FileWriter::DeleteFileX, cref(indexFile)));
    TEST(table.get(), ());

    uint64_t builderSize = 0;
    FilesContainerR cont(GetPlatform().GetReader(testFileName + DATA_FILE_EXTENSION));
    FeaturesVectorTest(cont).GetVector().ForEach([&builderSize](FeatureType const &, uint32_t)
    {
      ++builderSize;
    });
    TEST_EQUAL(builderSize, table->size(), ());

    table = unique_ptr<FeaturesOffsetsTable>();
    table = unique_ptr<FeaturesOffsetsTable>(FeaturesOffsetsTable::Load(indexFile));
    TEST_EQUAL(builderSize, table->size(), ());
  }

  UNIT_TEST(FeaturesOffsetsTable_ReadWrite)
  {
    string const testFileName = "test_file";
    Platform & pl = GetPlatform();

    FilesContainerR baseContainer(pl.GetReader("minsk-pass" DATA_FILE_EXTENSION));

    LocalCountryFile localFile = LocalCountryFile::MakeForTesting(testFileName);
    CountryIndexes::PreparePlaceOnDisk(localFile);

    string const indexFile = CountryIndexes::GetPath(localFile, CountryIndexes::Index::Offsets);
    FileWriter::DeleteFileX(indexFile);

    FeaturesOffsetsTable::Builder builder;
    FeaturesVector::ForEachOffset(baseContainer.GetReader(DATA_FILE_TAG), [&builder](uint32_t offset)
    {
      builder.PushOffset(offset);
    });

    unique_ptr<FeaturesOffsetsTable> table(FeaturesOffsetsTable::Build(builder));
    TEST(table.get(), ());
    TEST_EQUAL(builder.size(), table->size(), ());

    string const testFile = pl.WritablePathForFile(testFileName + DATA_FILE_EXTENSION);
    MY_SCOPE_GUARD(deleteTestFileGuard, bind(&FileWriter::DeleteFileX, cref(testFile)));
    MY_SCOPE_GUARD(deleteTestFileIndexGuard, bind(&FileWriter::DeleteFileX, cref(indexFile)));

    // Store table in a temporary data file.
    {
      FilesContainerW testContainer(testFile);

      // Just copy all sections except a possibly existing offsets
      // table section.
      baseContainer.ForEachTag([&baseContainer, &testContainer](string const & tag)
      {
        testContainer.Write(baseContainer.GetReader(tag), tag);
      });
      table->Save(indexFile);
      testContainer.Finish();
    }

    // Load table from the temporary data file.
    {
      unique_ptr<FeaturesOffsetsTable> loadedTable(FeaturesOffsetsTable::Load(indexFile));
      TEST(loadedTable.get(), ());

      TEST_EQUAL(table->size(), loadedTable->size(), ());
      for (uint64_t i = 0; i < table->size(); ++i)
        TEST_EQUAL(table->GetFeatureOffset(i), loadedTable->GetFeatureOffset(i), ());
    }
  }
}  // namespace feature
