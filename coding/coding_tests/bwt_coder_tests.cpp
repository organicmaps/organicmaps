#include "testing/testing.hpp"

#include "coding/bwt_coder.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <iterator>
#include <random>
#include <string>

using namespace coding;
using namespace std;

namespace
{
string EncodeDecode(BWTCoder::Params const & params, string const & s)
{
  vector<uint8_t> data;

  {
    MemWriter<decltype(data)> sink(data);
    BWTCoder::EncodeAndWrite(params, sink, s.size(), reinterpret_cast<uint8_t const *>(s.data()));
  }

  string result;
  {
    MemReader reader(data.data(), data.size());
    ReaderSource<MemReader> source(reader);

    BWTCoder::ReadAndDecode(source, back_inserter(result));
  }

  return result;
}

string EncodeDecodeBlock(string const & s)
{
  vector<uint8_t> data;

  {
    MemWriter<decltype(data)> sink(data);
    BWTCoder::EncodeAndWriteBlock(sink, s.size(), reinterpret_cast<uint8_t const *>(s.data()));
  }

  string result;
  {
    MemReader reader(data.data(), data.size());
    ReaderSource<MemReader> source(reader);

    auto const buffer = BWTCoder::ReadAndDecodeBlock(source);
    result.assign(buffer.begin(), buffer.end());
  }

  return result;
}

UNIT_TEST(BWTEncoder_Smoke)
{
  for (size_t blockSize = 1; blockSize < 100; ++blockSize)
  {
    BWTCoder::Params params;

    params.m_blockSize = blockSize;
    string const s = "abracadabra";
    TEST_EQUAL(s, EncodeDecodeBlock(s), ());
    TEST_EQUAL(s, EncodeDecode(params, s), (blockSize));
  }

  string const strings[] = {"", "mississippi", "again and again and again"};
  for (auto const & s : strings)
  {
    TEST_EQUAL(s, EncodeDecodeBlock(s), ());
    TEST_EQUAL(s, EncodeDecode(BWTCoder::Params{}, s), ());
  }
}

UNIT_TEST(BWT_Large)
{
  string s;
  for (size_t i = 0; i < 10000; ++i)
    s += "mississippi";
  TEST_EQUAL(s, EncodeDecode(BWTCoder::Params{}, s), ());
}

UNIT_TEST(BWT_AllBytes)
{
  int kSeed = 42;
  int kMin = 1;
  int kMax = 1000;

  mt19937 engine(kSeed);
  uniform_int_distribution<int> uid(kMin, kMax);

  string s;
  for (size_t i = 0; i < 256; ++i)
  {
    auto const count = uid(engine);
    ASSERT_GREATER_OR_EQUAL(count, kMin, ());
    ASSERT_LESS_OR_EQUAL(count, kMax, ());
    for (int j = 0; j < count; ++j)
      s.push_back(static_cast<uint8_t>(i));
  }
  shuffle(s.begin(), s.end(), engine);
  TEST_EQUAL(s, EncodeDecode(BWTCoder::Params{}, s), ());
}
}  // namespace
