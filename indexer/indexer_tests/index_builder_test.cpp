#include "../../testing/testing.hpp"
#include "../index.hpp"
#include "../index_builder.hpp"
#include "../classif_routine.hpp"
#include "../features_vector.hpp"
#include "../feature_processor.hpp"
#include "../../platform/platform.hpp"

UNIT_TEST(BuildIndexTest)
{
  classificator::Read(GetPlatform().ResourcesDir());
  string const dir = GetPlatform().WorkingDir();

  FileReader reader(dir + "minsk-pass.dat");
  // skip xml metadata header
  uint64_t startOffset = feature::ReadDatHeaderSize(reader);
  FileReader subReader = reader.SubReader(startOffset, reader.Size() - startOffset);
  FeaturesVector<FileReader> featuresVector(subReader);

  string serial;
  {
    MemWriter<string> serialWriter(serial);
    indexer::BuildIndex(featuresVector, serialWriter, "build_index_test");
  }

  MemReader indexReader(&serial[0], serial.size());
  Index<FileReader, MemReader>::Type index;
  index.Add(reader, indexReader);
}
