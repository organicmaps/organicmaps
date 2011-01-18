#include "../../testing/testing.hpp"

#include "../file_container.hpp"
#include "../varint.hpp"

#include "../../base/string_utils.hpp"


UNIT_TEST(FilesContainer_Smoke)
{
  string const fName = "file_container.tmp";
  FileWriter::DeleteFile(fName);
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
  FileWriter::DeleteFile(fName);
}

namespace
{
  void CheckInvariant(FilesContainerR & reader, string const & tag, int64_t test)
  {
    FileReader r = reader.GetReader(tag);
    TEST_EQUAL(test, ReadPrimitiveFromPos<int64_t>(r, 0), ());
  }
}

UNIT_TEST(FilesContainer_Shared)
{
  string const fName = "file_container.tmp";
  FileWriter::DeleteFile(fName);

  uint32_t const count = 10;
  int64_t const test64 = 908175281437210836;

  {
    // shared container fill

    FilesContainerW writer(fName);

    FileWriter w1 = writer.GetWriter("1");
    WriteToSink(w1, uint32_t(0));

    for (uint32_t i = 0; i < count; ++i)
      WriteVarUint(w1, i);
    w1.Flush();

    FileWriter w2 = writer.GetWriter("2");
    WriteToSink(w2, test64);
    w2.Flush();

    writer.Finish();
  }

  {
    // shared container read and fill

    FilesContainerR reader(fName);
    FileReader r1 = reader.GetReader("1");
    uint64_t const offset = sizeof(uint32_t);
    r1 = r1.SubReader(offset, r1.Size() - offset);

    CheckInvariant(reader, "2", test64);

    FilesContainerW writer(fName, FileWriter::OP_APPEND);
    FileWriter w = writer.GetWriter("3");

    ReaderSource<FileReader> src(r1);
    for (uint32_t i = 0; i < count; ++i)
    {
      uint32_t test = ReadVarUint<uint32_t>(src);
      TEST_EQUAL(test, i, ());
      WriteVarUint(w, i);
    }

    w.Flush();
    writer.Finish();
  }

  {
    // check invariant
    FilesContainerR reader(fName);
    CheckInvariant(reader, "2", test64);
  }

  FileWriter::DeleteFile(fName);
}
