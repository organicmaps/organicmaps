#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "coding/text_storage.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

using namespace coding;
using namespace std;

namespace
{
template <typename Engine>
string GenerateRandomString(Engine & engine)
{
  int const kMinLength = 0;
  int const kMaxLength = 400;

  int const kMinByte = 0;
  int const kMaxByte = 255;

  uniform_int_distribution<int> length(kMinLength, kMaxLength);
  uniform_int_distribution<int> byte(kMinByte, kMaxByte);
  string s(length(engine), '\0');
  for (auto & b : s)
    b = byte(engine);
  return s;
}

void DumpStrings(vector<string> const & strings, uint64_t blockSize, vector<uint8_t> & buffer)
{
  MemWriter<vector<uint8_t>> writer(buffer);
  BlockedTextStorageWriter<decltype(writer)> ts(writer, blockSize);
  for (auto const & s : strings)
    ts.Append(s);
}

UNIT_TEST(TextStorage_Smoke)
{
  vector<uint8_t> buffer;
  DumpStrings({} /* strings */, 10 /* blockSize */, buffer);

  {
    MemReader reader(buffer.data(), buffer.size());
    BlockedTextStorageIndex index;
    index.Read(reader);
    TEST_EQUAL(index.GetNumStrings(), 0, ());
    TEST_EQUAL(index.GetNumBlockInfos(), 0, ());
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    BlockedTextStorage<decltype(reader)> ts(reader);
    TEST_EQUAL(ts.GetNumStrings(), 0, ());
  }
}

UNIT_TEST(TextStorage_Simple)
{
  vector<string> const strings = {{"", "Hello", "Hello, World!", "Hola mundo", "Smoke test"}};

  vector<uint8_t> buffer;
  DumpStrings(strings, 10 /* blockSize */, buffer);

  {
    MemReader reader(buffer.data(), buffer.size());
    BlockedTextStorageIndex index;
    index.Read(reader);
    TEST_EQUAL(index.GetNumStrings(), strings.size(), ());
    TEST_EQUAL(index.GetNumBlockInfos(), 3, ());
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    BlockedTextStorage<decltype(reader)> ts(reader);
    TEST_EQUAL(ts.GetNumStrings(), strings.size(), ());
    for (size_t i = 0; i < ts.GetNumStrings(); ++i)
      TEST_EQUAL(ts.ExtractString(i), strings[i], ());
  }
}

UNIT_TEST(TextStorage_Empty)
{
  vector<string> strings;
  for (int i = 0; i < 1000; ++i)
  {
    strings.emplace_back(string(1 /* size */, i % 256));
    for (int j = 0; j < 1000; ++j)
      strings.emplace_back();
  }

  vector<uint8_t> buffer;
  DumpStrings(strings, 5 /* blockSize */, buffer);

  {
    MemReader reader(buffer.data(), buffer.size());
    BlockedTextStorage<decltype(reader)> ts(reader);
    TEST_EQUAL(ts.GetNumStrings(), strings.size(), ());
    for (size_t i = 0; i < ts.GetNumStrings(); ++i)
      TEST_EQUAL(ts.ExtractString(i), strings[i], ());
  }
}

UNIT_TEST(TextStorage_Random)
{
  int const kSeed = 42;
  int const kNumStrings = 1000;
  int const kBlockSize = 100;
  mt19937 engine(kSeed);

  vector<string> strings;
  for (int i = 0; i < kNumStrings; ++i)
    strings.push_back(GenerateRandomString(engine));

  vector<uint8_t> buffer;
  DumpStrings(strings, kBlockSize, buffer);

  MemReader reader(buffer.data(), buffer.size());
  BlockedTextStorage<decltype(reader)> ts(reader);

  TEST_EQUAL(ts.GetNumStrings(), strings.size(), ());
  for (size_t i = 0; i < ts.GetNumStrings(); ++i)
    TEST_EQUAL(ts.ExtractString(i), strings[i], ());

  for (size_t i = ts.GetNumStrings() - 1; i < ts.GetNumStrings(); --i)
    TEST_EQUAL(ts.ExtractString(i), strings[i], ());
}
}  // namespace
