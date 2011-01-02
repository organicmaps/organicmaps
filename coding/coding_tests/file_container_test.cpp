#include "../../testing/testing.hpp"

#include "../file_container.hpp"
#include "../varint.hpp"

#include "../../base/string_utils.hpp"


UNIT_TEST(FileContainer_Smoke)
{
  string const fName = "file_container.tmp";
  size_t const count = 10;

  // fill container one by one
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

  // read container one by one
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

  // append to container
  uint32_t const arrAppend[] = { 666, 777, 888 };
  for (size_t i = 0; i < ARRAY_SIZE(arrAppend); ++i)
  {
    {
      FilesContainerW writer(fName, FileWriter::OP_APPEND);

      FileWriter w = writer.GetWriter(utils::to_string(arrAppend[i]));
      WriteVarUint(w, arrAppend[i]);

      writer.Finish();
    }

    // read appended
    {
      FilesContainerR reader(fName);

      FileReader r = reader.GetReader(utils::to_string(arrAppend[i]));
      ReaderSource<FileReader> src(r);

      uint32_t const test = ReadVarUint<uint32_t>(src);
      CHECK_EQUAL(arrAppend[i], test, ());
    }
  }
}
