#include "testing/testing.hpp"

#include "indexer/data_source.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/index_builder.hpp"
#include "indexer/scale_index.hpp"

#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "defines.hpp"

#include "platform/platform.hpp"

#include "coding/files_container.hpp"

#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"

#include <string>
#include <vector>

namespace index_builder_test
{
using namespace generator::tests_support;
using namespace std;

UNIT_CLASS_TEST(TestWithClassificator, BuildIndexTest)
{
  Platform & p = GetPlatform();

  FilesContainerR originalContainer(p.GetReader("minsk-pass" DATA_FILE_EXTENSION));

  // Build index.
  vector<char> serialIndex;
  {
    FeaturesVectorTest features(originalContainer);

    MemWriter<vector<char>> serialWriter(serialIndex);
    indexer::BuildIndex(features.GetHeader(), features.GetVector(), serialWriter, "build_index_test");
  }

  // Create a new mwm file.
  string const fileName = "build_index_test" DATA_FILE_EXTENSION;
  string const filePath = p.WritablePathForFile(fileName);
  SCOPE_GUARD(cleanup, [&] { FileWriter::DeleteFileX(filePath); });

  // Copy original mwm file and replace index in it.
  {
    FilesContainerW containerWriter(filePath);
    vector<string> tags;
    originalContainer.ForEachTag(base::MakeBackInsertFunctor(tags));
    for (auto const & tag : tags)
      if (tag != INDEX_FILE_TAG)
        containerWriter.Write(originalContainer.GetReader(tag), tag);
    containerWriter.Write(serialIndex, INDEX_FILE_TAG);
  }

  {
    FilesContainerR const container(filePath);
    ScaleIndex<ModelReaderPtr> const newIndex(container.GetReader(INDEX_FILE_TAG));
    TEST(newIndex.GetVersion() == ScaleIndexBase::Version::Latest, ());
  }

  {
    FrozenDataSource dataSource;
    auto const result = dataSource.Register(platform::LocalCountryFile::MakeForTesting("build_index_test")).second;
    TEST_EQUAL(result, MwmSet::RegResult::Success, ());
    size_t featureCount = 0;
    dataSource.ForEachInScale([&](FeatureType &) { ++featureCount; }, 15);
    TEST_GREATER(featureCount, 0, ());
  }
}
}  // namespace index_builder_test
