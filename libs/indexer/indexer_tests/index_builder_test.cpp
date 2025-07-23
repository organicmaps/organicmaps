#include "testing/testing.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/index_builder.hpp"
#include "indexer/scales.hpp"

#include "defines.hpp"

#include "platform/platform.hpp"

#include "coding/files_container.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <string>
#include <vector>

using namespace std;

UNIT_TEST(BuildIndexTest)
{
  Platform & p = GetPlatform();
  classificator::Load();

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
  FileWriter::DeleteFileX(filePath);

  // Copy original mwm file and replace index in it.
  {
    FilesContainerW containerWriter(filePath);
    vector<string> tags;
    originalContainer.ForEachTag(base::MakeBackInsertFunctor(tags));
    for (size_t i = 0; i < tags.size(); ++i)
      if (tags[i] != INDEX_FILE_TAG)
        containerWriter.Write(originalContainer.GetReader(tags[i]), tags[i]);
    containerWriter.Write(serialIndex, INDEX_FILE_TAG);
  }

  {
    // Check that index actually works.
    FrozenDataSource dataSource;
    UNUSED_VALUE(dataSource.Register(platform::LocalCountryFile::MakeForTesting("build_index_test")));

    // Make sure that index is actually parsed.
    dataSource.ForEachInScale([](FeatureType &) { return; }, 15);
  }

  // Clean after the test.
  FileWriter::DeleteFileX(filePath);
}
