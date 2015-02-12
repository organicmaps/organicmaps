#include "../features_offsets_table.hpp"
#include "../data_header.hpp"
#include "../features_vector.hpp"
#include "../../coding/file_container.hpp"
#include "../../base/scope_guard.hpp"
#include "../../std/bind.hpp"
#include "../../std/string.hpp"
#include "../../defines.hpp"
#include "../../platform/platform.hpp"
#include "../../testing/testing.hpp"

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
  }

  UNIT_TEST(FeaturesOffsetsTable_ReadWrite)
  {
    Platform & p = GetPlatform();
    FilesContainerR baseContainer(p.GetReader("minsk-pass" DATA_FILE_EXTENSION));

    feature::DataHeader header;
    header.Load(baseContainer.GetReader(HEADER_FILE_TAG));

    FeaturesVector features(baseContainer, header);

    FeaturesOffsetsTable::Builder builder;
    features.ForEachOffset([&builder](FeatureType /* type */, uint64_t offset)
                           {
                             builder.PushOffset(offset);
                           });

    unique_ptr<FeaturesOffsetsTable> table(FeaturesOffsetsTable::Build(builder));
    TEST(table.get(), ());
    TEST_EQUAL(builder.size(), table->size(), ());

    string const testFile = p.WritablePathForFile("test_file" DATA_FILE_EXTENSION);
    MY_SCOPE_GUARD(deleteTestFileGuard, bind(&FileWriter::DeleteFileX, cref(testFile)));

    // Store table in a temporary data file.
    {
      FilesContainerW testContainer(testFile);

      // Just copy all sections except a possibly existing offsets
      // table section.
      baseContainer.ForEachTag([&baseContainer, &testContainer](string const & tag)
                               {
                                 if (tag != FEATURES_OFFSETS_TABLE_FILE_TAG)
                                   testContainer.Write(baseContainer.GetReader(tag), tag);
                               });
      table->Save(testContainer);
      testContainer.Finish();
    }

    // Load table from the temporary data file.
    {
      FilesMappingContainer testContainer(testFile);
      MY_SCOPE_GUARD(testContainerGuard, bind(&FilesMappingContainer::Close, &testContainer));

      unique_ptr<FeaturesOffsetsTable> loadedTable(FeaturesOffsetsTable::Load(testContainer));
      TEST(loadedTable.get(), ());

      TEST_EQUAL(table->size(), loadedTable->size(), ());
      for (uint64_t i = 0; i < table->size(); ++i)
        TEST_EQUAL(table->GetFeatureOffset(i), loadedTable->GetFeatureOffset(i), ());
    }
  }
}  // namespace feature
