#include "../../testing/testing.hpp"
#include "../index.hpp"
#include "../index_builder.hpp"
#include "../../platform/platform.hpp"
#include "../../coding/file_container.hpp"
#include "../../base/macros.hpp"
#include "../../base/stl_add.hpp"
#include "../../std/string.hpp"


UNIT_TEST(IndexParseTest)
{
  Index<FileReader>::Type index;
  index.Add(GetPlatform().WritablePathForFile("minsk-pass" DATA_FILE_EXTENSION));

  // Make sure that index is actually parsed.
  index.ForEachInScale(NoopFunctor(), 15);
}
