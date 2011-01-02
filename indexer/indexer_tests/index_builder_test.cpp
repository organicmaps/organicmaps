#include "../../testing/testing.hpp"

#include "../index.hpp"
#include "../index_builder.hpp"
#include "../classif_routine.hpp"
#include "../features_vector.hpp"
#include "../data_header_reader.hpp"

#include "../../coding/file_container.hpp"

#include "../../platform/platform.hpp"


UNIT_TEST(BuildIndexTest)
{
  Platform & p = GetPlatform();
  classificator::Read(p.ReadPathForFile("drawing_rules.bin"),
                      p.ReadPathForFile("classificator.txt"),
                      p.ReadPathForFile("visibility.txt"));

  FilesContainerR container(p.WritablePathForFile("minsk-pass" DATA_FILE_EXTENSION));

  FeaturesVector<FileReader> featuresVector(container);

  string serial;
  {
    MemWriter<string> serialWriter(serial);
    indexer::BuildIndex(featuresVector, serialWriter, "build_index_test");
  }

  MemReader indexReader(&serial[0], serial.size());
  Index<FileReader, MemReader>::Type index;
  index.Add(FeatureReaders<FileReader>(container), indexReader);
}
