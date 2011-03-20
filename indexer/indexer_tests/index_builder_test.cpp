#include "../../testing/testing.hpp"

#include "../index.hpp"
#include "../index_builder.hpp"
#include "../classificator_loader.hpp"
#include "../features_vector.hpp"
#include "../../defines.hpp"
#include "../../platform/platform.hpp"
#include "../../coding/file_container.hpp"
#include "../../base/stl_add.hpp"


UNIT_TEST(BuildIndexTest)
{
  Platform & p = GetPlatform();
  classificator::Read(p.ReadPathForFile("drawing_rules.bin"),
                      p.ReadPathForFile("classificator.txt"),
                      p.ReadPathForFile("visibility.txt"));

  FilesContainerR originalContainer(p.WritablePathForFile("minsk-pass" DATA_FILE_EXTENSION));

  // Build index.
  vector<char> serialIndex;
  {
    FeaturesVector featuresVector(originalContainer);
    MemWriter<vector<char> > serialWriter(serialIndex);
    indexer::BuildIndex(ScaleIndexBase::NUM_BUCKETS, featuresVector, serialWriter, "build_index_test");
  }

  // Create a new mwm file.
  string const fileName = "build_index_test" DATA_FILE_EXTENSION;
  FileWriter::DeleteFileX(fileName);

  // Copy original mwm file and replace index in it.
  {
    FilesContainerW containerWriter(fileName);
    vector<string> tags;
    originalContainer.ForEachTag(MakeBackInsertFunctor(tags));
    for (size_t i = 0; i < tags.size(); ++i)
    {
      if (tags[i] != INDEX_FILE_TAG)
      {
        FileReader reader = originalContainer.GetReader(tags[i]);
        size_t const sz = static_cast<size_t>(reader.Size());
        if (sz > 0)
        {
          vector<char> data(sz);
          reader.Read(0, &data[0], sz);
          containerWriter.Append(data, tags[i]);
        }
      }
    }
    containerWriter.Append(serialIndex, INDEX_FILE_TAG);
  }

  {
    // Check that index actually works.
    Index<FileReader>::Type index;
    index.Add(fileName);

    // Make sure that index is actually parsed.
    index.ForEachInScale(NoopFunctor(), 15);
  }

  // Clean after the test.
  FileWriter::DeleteFileX(fileName);
}
