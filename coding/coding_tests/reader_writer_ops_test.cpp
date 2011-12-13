#include "../../base/SRC_FIRST.hpp"

#include "../../testing/testing.hpp"

#include "../../coding/reader_writer_ops.hpp"
#include "../../coding/file_reader.hpp"
#include "../../coding/file_writer.hpp"

#include "../../std/algorithm.hpp"


namespace
{
  void GetReverseForReaderAndTmpFile(Reader const & src, vector<char> & buffer)
  {
    char const * tmpFile = "reversed_file.tmp";

    {
      FileWriter writer(tmpFile);
      rw_ops::Reverse(src, writer);
    }

    {
      FileReader reader(tmpFile);
      buffer.clear();
      MemWriter<vector<char> > writer(buffer);
      rw_ops::Reverse(reader, writer);
    }

    FileWriter::DeleteFileX(tmpFile);
  }

  void FillRandFile(string const & fName, size_t count)
  {
    FileWriter writer(fName);

    srand(666);

    while (count-- > 0)
    {
      char const c = rand();
      writer.Write(&c, 1);
    }
  }
}

UNIT_TEST(Reverse_Smoke)
{
  {
    char arr[] = { 0xA, 0xB, 0xC, 0xD, 0xF };
    size_t const sz = ARRAY_SIZE(arr);

    MemReader reader(&arr[0], sz);

    vector<char> buffer;
    GetReverseForReaderAndTmpFile(reader, buffer);

    TEST_EQUAL(buffer.size(), ARRAY_SIZE(arr), ());
    TEST(equal(arr, arr + ARRAY_SIZE(arr), buffer.begin()), ());
  }

  char const * tmpFile = "random_file.tmp";

  {
    FillRandFile(tmpFile, 10 * 1024 + 527);
    FileReader reader(tmpFile);

    vector<char> buffer;
    GetReverseForReaderAndTmpFile(reader, buffer);

    string str;
    reader.ReadAsString(str);
    TEST_EQUAL(str.size(), buffer.size(), ());
    TEST(equal(str.begin(), str.end(), buffer.begin()), ());

    FileWriter::DeleteFileX(tmpFile);
  }
}
