#include "../../base/SRC_FIRST.hpp"

#include "../index.hpp"
#include "../index_builder.hpp"

#include "../../testing/testing.hpp"
#include "../../coding/file_reader.hpp"
#include "../../coding/writer.hpp"
#include "../../platform/platform.hpp"

#include "../../std/string.hpp"

UNIT_TEST(IndexParseTest)
{
  FileReader dataReader(GetPlatform().WritablePathForFile("minsk-pass.dat"));
  FileReader indexReader(GetPlatform().WritablePathForFile("minsk-pass.dat.idx"));

  Index<FileReader, FileReader>::Type index;
  index.Add(dataReader, indexReader);
}
