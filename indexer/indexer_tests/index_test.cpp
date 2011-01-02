#include "../../base/SRC_FIRST.hpp"

#include "../index.hpp"
#include "../index_builder.hpp"

#include "../../testing/testing.hpp"

#include "../../coding/file_container.hpp"

#include "../../platform/platform.hpp"

#include "../../std/string.hpp"


UNIT_TEST(IndexParseTest)
{
  FilesContainerR container(GetPlatform().WritablePathForFile("minsk-pass" DATA_FILE_EXTENSION));

  Index<FileReader, FileReader>::Type index;
  index.Add(FeatureReaders<FileReader>(container), container.GetReader(INDEX_FILE_TAG));
}
