#include "../../testing/testing.hpp"

#include "../file_container.hpp"
#include "../varint.hpp"

#include "../../base/string_utils.hpp"


UNIT_TEST(FileContainer_Smoke)
{
  string const fName = "file_container.tmp";
  size_t const count = 10;

  {
    FilesContainerW writer(fName);

    for (size_t i = 0; i < count; ++i)
    {
      FileWriter w = writer.GetWriter(utils::to_string(i));

      for (uint32_t j = 0; j < i; ++j)
        WriteVarUint(w, j);
    }

    writer.Finish();
  }

  {
    FilesContainerR reader(fName);

    for (size_t i = 0; i < count; ++i)
    {
      FileReader r = reader.GetReader(utils::to_string(i));
      ReaderSource<FileReader> src(r);

      for (uint32_t j = 0; j < i; ++j)
      {
        uint32_t const test = ReadVarUint<uint32_t>(src);
        CHECK_EQUAL(j, test, ());
      }
    }
  }
}
