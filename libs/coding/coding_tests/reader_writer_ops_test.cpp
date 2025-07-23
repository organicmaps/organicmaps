#include "testing/testing.hpp"

#include "coding/byte_stream.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader_writer_ops.hpp"

#include "base/random.hpp"

#include <algorithm>
#include <vector>

namespace rw_ops_tests
{
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
    MemWriter<vector<char>> writer(buffer);
    rw_ops::Reverse(reader, writer);
  }

  FileWriter::DeleteFileX(tmpFile);
}

void FillRandFile(string const & fName, size_t count)
{
  FileWriter writer(fName);

  base::UniformRandom<int8_t> rand;

  while (count-- > 0)
  {
    int8_t const c = rand();
    writer.Write(&c, 1);
  }
}
}  // namespace

UNIT_TEST(Reverse_Smoke)
{
  {
    char arr[] = {0xA, 0xB, 0xC, 0xD, 0xF};
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
}  // namespace

UNIT_TEST(ReadWrite_POD)
{
  base::UniformRandom<uint32_t> rand;

  size_t const count = 1000;
  vector<ThePOD> src(1000);
  for (size_t i = 0; i < count; ++i)
  {
    src[i].m_i = rand();
    src[i].m_d = double(rand()) / double(rand());
  }

  vector<char> buffer;
  PushBackByteSink<vector<char>> sink(buffer);
  rw::WriteVectorOfPOD(sink, src);

  buffer_vector<ThePOD, 128> dest;
  ArrayByteSource byteSrc(buffer.data());
  rw::ReadVectorOfPOD(byteSrc, dest);

  TEST(equal(src.begin(), src.end(), dest.begin()), ());
}

namespace
{
template <class T>
void TestIntegral()
{
  std::vector<T> ethalon{static_cast<T>(-1),           0, 1, static_cast<T>(-2), 2, std::numeric_limits<T>::min(),
                         std::numeric_limits<T>::max()};

  std::string buffer;
  MemWriter writer(buffer);
  rw::Write(writer, ethalon);

  std::vector<T> expected;
  MemReader reader(buffer);
  ReaderSource src(reader);
  rw::Read(src, expected);

  TEST_EQUAL(ethalon, expected, ());
}
}  // namespace

UNIT_TEST(ReadWrite_Integral)
{
  TestIntegral<uint32_t>();
  TestIntegral<int32_t>();
  TestIntegral<uint64_t>();
  TestIntegral<int64_t>();
}

}  // namespace rw_ops_tests
