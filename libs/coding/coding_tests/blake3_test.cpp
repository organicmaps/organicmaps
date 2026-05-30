#include "testing/testing.hpp"

#include "coding/base64.hpp"
#include "coding/blake3.hpp"
#include "coding/hex.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace blake3_test
{
using coding::Blake3;

// Official BLAKE3 test-vector input of a given length: input[i] = i % 251.
std::vector<uint8_t> MakeVectorInput(size_t len)
{
  std::vector<uint8_t> v(len);
  for (size_t i = 0; i < len; ++i)
    v[i] = static_cast<uint8_t>(i % 251);
  return v;
}

// Known answers from 3party/BLAKE3/test_vectors/test_vectors.json (first 32 bytes of each output).
// Lengths cover the empty input, sub-block, exact chunk (1024) and multi-chunk tree-merge cases.
UNIT_TEST(BLAKE3_KnownAnswers)
{
  struct
  {
    size_t m_len;
    char const * m_hashHex;
  } const kVectors[] = {
      {0, "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262"},
      {1, "2d3adedff11b61f14c886e35afa036736dcd87a74d27b5c1510225d0f592e213"},
      {2, "7b7015bb92cf0b318037702a6cdd81dee41224f734684c2c122cd6359cb1ee63"},
      {1023, "10108970eeda3eb932baac1428c7a2163b0e924c9a9e25b35bba72b28f70bd11"},
      {1024, "42214739f095a406f3fc83deb889744ac00df831c10daa55189b5d121c855af7"},
      {1025, "d00278ae47eb27b34faecf67b4fe263f82d5412916c1ffd97c8cb7fb814b8444"},
      {2048, "e776b6028c7cd22a4d0ba182a8bf62205d2ef576467e838ed6f2529b85fba24a"},
      {3072, "b98cb0ff3623be03326b373de6b9095218513e64f1ee2edd2525c7ad1e5cffd2"},
  };

  for (auto const & v : kVectors)
  {
    auto const input = MakeVectorInput(v.m_len);
    Blake3 hasher;
    if (!input.empty())
      hasher.Update(input.data(), input.size());
    auto const hash = hasher.Finalize();
    // Compare as uppercase hex on both sides for a readable mismatch message.
    TEST_EQUAL(ToHex(hash), ToHex(FromHex(v.m_hashHex)), (v.m_len));
  }
}

// Incremental hashing across arbitrary chunk boundaries must equal one-shot hashing.
UNIT_TEST(BLAKE3_Streaming)
{
  std::vector<uint8_t> data(100003);
  for (size_t i = 0; i < data.size(); ++i)
    data[i] = static_cast<uint8_t>((i * 1103515245u + 12345u) >> 16);

  Blake3 oneShot;
  oneShot.Update(data.data(), data.size());
  auto const expected = oneShot.Finalize();

  // Feed growing irregular pieces that cross 64-byte block and 1024-byte chunk boundaries.
  Blake3 streamed;
  size_t pos = 0;
  size_t step = 1;
  while (pos < data.size())
  {
    size_t const n = std::min(step, data.size() - pos);
    streamed.Update(data.data() + pos, n);
    pos += n;
    step = step * 2 + 1;
  }
  TEST_EQUAL(streamed.Finalize(), expected, ());

  TEST_EQUAL(Blake3::CalculateForString(std::string_view(reinterpret_cast<char const *>(data.data()), data.size())),
             expected, ());
}

// File hashing matches in-memory hashing; truncated base64 is a prefix of the full digest.
UNIT_TEST(BLAKE3_File_And_Truncation)
{
  std::string const contents =
      "Organic Maps is the ultimate companion app for travellers, tourists, hikers, and cyclists!";
  platform::tests_support::ScopedFile sf("blake3_test.tmp", contents);
  auto const & path = sf.GetFullPath();

  auto const memHash = Blake3::CalculateForString(contents);
  TEST_EQUAL(Blake3::Calculate(path), memHash, ());

  std::string_view const memView(reinterpret_cast<char const *>(memHash.data()), memHash.size());

  // Full base64 digest.
  TEST_EQUAL(Blake3::CalculateBase64(path), base64::Encode(memView), ());

  // Truncated (compact) digest used in countries.json: 9 bytes -> 12 base64 chars, no padding.
  size_t constexpr kShortBytes = 9;
  std::string const shortB64 = Blake3::CalculateBase64(path, kShortBytes);
  TEST_EQUAL(shortB64, base64::Encode(memView.substr(0, kShortBytes)), ());
  TEST_EQUAL(shortB64.size(), 12, ());
  TEST_EQUAL(shortB64.find('='), std::string::npos, ());

  // Streaming FinalizeToBase64 must agree with the one-shot truncated form.
  Blake3 hasher;
  hasher.Update(contents.data(), contents.size());
  TEST_EQUAL(hasher.FinalizeToBase64(kShortBytes), shortB64, ());
}
}  // namespace blake3_test
