#include "testing/testing.hpp"

#include "coding/reader_writer_ops.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/byte_stream.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

using namespace std;

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

  {
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
    }

    FileWriter::DeleteFileX(tmpFile);
  }
}

namespace
{
  struct ThePOD
  {
    uint32_t m_i;
    double m_d;
  };

  bool operator==(ThePOD const & r1, ThePOD const & r2)
  {
    return (r1.m_i == r2.m_i && r1.m_d == r2.m_d);
  }
}

UNIT_TEST(ReadWrite_POD)
{
  srand(666);

  size_t const count = 1000;
  vector<ThePOD> src(1000);
  for (size_t i = 0; i < count; ++i)
  {
    src[i].m_i = rand();
    src[i].m_d = double(rand()) / double(rand());
  }

  vector<char> buffer;
  PushBackByteSink<vector<char> > sink(buffer);
  rw::WriteVectorOfPOD(sink, src);

  buffer_vector<ThePOD, 128> dest;
  ArrayByteSource byteSrc(buffer.data());
  rw::ReadVectorOfPOD(byteSrc, dest);

  TEST(equal(src.begin(), src.end(), dest.begin()), ());
}
