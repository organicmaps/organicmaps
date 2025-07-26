#include "testing/testing.hpp"

#include "coding/byte_stream.hpp"
#include "coding/hex.hpp"
#include "coding/reader.hpp"
#include "coding/var_serial_vector.hpp"
#include "coding/writer.hpp"

#include "base/macros.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

using namespace std;

char const kHexSerial[] =
    "03000000"
    "01000000"
    "04000000"
    "06000000"
    "616263646566";

namespace
{

template <typename ItT, typename TDstStream>
void WriteVarSerialVector(ItT begin, ItT end, TDstStream & dst)
{
  vector<uint32_t> offsets;
  uint32_t offset = 0;
  for (ItT it = begin; it != end; ++it)
  {
    offset += it->size() * sizeof((*it)[0]);
    offsets.push_back(offset);
  }

  WriteToSink(dst, static_cast<uint32_t>(end - begin));

  for (size_t i = 0; i < offsets.size(); ++i)
    WriteToSink(dst, offsets[i]);

  for (ItT it = begin; it != end; ++it)
  {
    typename ItT::value_type const & v = *it;
    if (!v.empty())
      dst.Write(&v[0], v.size() * sizeof(v[0]));
  }
}

}  // namespace

UNIT_TEST(WriteSerial)
{
  vector<string> elements;
  elements.push_back("a");
  elements.push_back("bcd");
  elements.push_back("ef");

  string output;
  PushBackByteSink<string> sink(output);
  WriteVarSerialVector(elements.begin(), elements.end(), sink);

  TEST_EQUAL(ToHex(output), kHexSerial, ());
}

UNIT_TEST(WriteSerialWithWriter)
{
  string output;
  MemWriter<string> writer(output);
  VarSerialVectorWriter<MemWriter<string>> recordWriter(writer, 3);
  writer.Write("a", 1);
  recordWriter.FinishRecord();
  writer.Write("bcd", 3);
  recordWriter.FinishRecord();
  writer.Write("ef", 2);
  recordWriter.FinishRecord();
  TEST_EQUAL(ToHex(output), kHexSerial, ());
}

UNIT_TEST(ReadSerial)
{
  string serial(FromHex(string(kHexSerial)));
  MemReader memReader(&serial[0], serial.size());
  ReaderSource<MemReader> memSource(memReader);
  VarSerialVectorReader<MemReader> reader(memSource);

  TEST_EQUAL(reader.Read(0), "a", ());
  TEST_EQUAL(reader.Read(1), "bcd", ());
  TEST_EQUAL(reader.Read(2), "ef", ());
}

UNIT_TEST(EncodeDecode)
{
  mt19937 rng(0);
  vector<string> elements;

  for (size_t i = 0; i < 1024; ++i)
  {
    string s(1 + (rng() % 20), 0);
    for (size_t j = 0; j < s.size(); ++j)
      s[j] = static_cast<char>(rng() % 26) + 'a';
    elements.push_back(s);
  }

  string serial;
  PushBackByteSink<string> sink(serial);
  WriteVarSerialVector(elements.begin(), elements.end(), sink);

  MemReader memReader(serial.c_str(), serial.size());
  ReaderSource<MemReader> memSource(memReader);
  VarSerialVectorReader<MemReader> reader(memSource);

  for (size_t i = 0; i < elements.size(); ++i)
    TEST_EQUAL(reader.Read(static_cast<uint32_t>(i)), elements[i], ());
}
