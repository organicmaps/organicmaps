#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "coding/text_storage.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace
{
template <typename Engine>
std::string GenerateRandomString(Engine & engine)
{
  int const kMinLength = 0;
  int const kMaxLength = 400;

  int const kMinByte = 0;
  int const kMaxByte = 255;

  std::uniform_int_distribution<int> length(kMinLength, kMaxLength);
  std::uniform_int_distribution<int> byte(kMinByte, kMaxByte);
  std::string s(length(engine), '\0');
  for (auto & b : s)
    b = byte(engine);
  return s;
}

void DumpStrings(std::vector<std::string> const & strings, uint64_t blockSize, std::vector<uint8_t> & buffer)
{
  MemWriter<std::vector<uint8_t>> writer(buffer);
  coding::BlockedTextStorageWriter<decltype(writer)> ts(writer, blockSize);
  for (auto const & s : strings)
    ts.Append(s);
}

UNIT_TEST(TextStorage_Smoke)
{
  std::vector<uint8_t> buffer;
  DumpStrings({} /* strings */, 10 /* blockSize */, buffer);

  {
    MemReader reader(buffer.data(), buffer.size());
    coding::BlockedTextStorageIndex index;
    index.Read(reader);
    TEST_EQUAL(index.GetNumStrings(), 0, ());
    TEST_EQUAL(index.GetNumBlockInfos(), 0, ());
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    coding::BlockedTextStorage<decltype(reader)> ts(reader);
    TEST_EQUAL(ts.GetNumStrings(), 0, ());
  }
}

UNIT_TEST(TextStorage_Simple)
{
  std::vector<std::string> const strings = {{"", "Hello", "Hello, World!", "Hola mundo", "Smoke test"}};

  std::vector<uint8_t> buffer;
  DumpStrings(strings, 10 /* blockSize */, buffer);

  {
    MemReader reader(buffer.data(), buffer.size());
    coding::BlockedTextStorageIndex index;
    index.Read(reader);
    TEST_EQUAL(index.GetNumStrings(), strings.size(), ());
    TEST_EQUAL(index.GetNumBlockInfos(), 3, ());
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    coding::BlockedTextStorage<decltype(reader)> ts(reader);
    TEST_EQUAL(ts.GetNumStrings(), strings.size(), ());
    for (size_t i = 0; i < ts.GetNumStrings(); ++i)
      TEST_EQUAL(ts.ExtractString(i), strings[i], ());
  }
}

UNIT_TEST(TextStorage_Empty)
{
  std::vector<std::string> strings;
  for (int i = 0; i < 1000; ++i)
  {
    strings.emplace_back(std::string(1 /* size */, i % 256));
    for (int j = 0; j < 1000; ++j)
      strings.emplace_back();
  }

  std::vector<uint8_t> buffer;
  DumpStrings(strings, 5 /* blockSize */, buffer);

  {
    MemReader reader(buffer.data(), buffer.size());
    coding::BlockedTextStorage<decltype(reader)> ts(reader);
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
  std::mt19937 engine(kSeed);

  std::vector<std::string> strings;
  for (int i = 0; i < kNumStrings; ++i)
    strings.push_back(GenerateRandomString(engine));

  std::vector<uint8_t> buffer;
  DumpStrings(strings, kBlockSize, buffer);

  MemReader reader(buffer.data(), buffer.size());
  coding::BlockedTextStorage<decltype(reader)> ts(reader);

  TEST_EQUAL(ts.GetNumStrings(), strings.size(), ());
  for (size_t i = 0; i < ts.GetNumStrings(); ++i)
    TEST_EQUAL(ts.ExtractString(i), strings[i], ());

  for (size_t i = ts.GetNumStrings() - 1; i < ts.GetNumStrings(); --i)
    TEST_EQUAL(ts.ExtractString(i), strings[i], ());
}
}  // namespace
