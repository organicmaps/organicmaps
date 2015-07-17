#include "testing/testing.hpp"

#include "coding/compressed_bit_vector.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "std/random.hpp"

uint32_t const NUMS_COUNT = 12345;

UNIT_TEST(CompressedBitVector_Sparse)
{
  mt19937 rng(0);
  vector<uint32_t> posOnes;
  uint32_t sum = 0;
  for (uint32_t i = 0; i < NUMS_COUNT; ++i)
  {
    uint32_t byteSize = rng() % 2 + 1;
    uint64_t num = rng() & ((uint64_t(1) << (byteSize * 7)) - 1);
    if (num == 0) num = 1;
    sum += num;
    posOnes.push_back(sum);
  }
  for (uint32_t j = 0; j < 5; ++j)
  {
    if (j == 1) posOnes.insert(posOnes.begin(), 1, 0);
    if (j == 2) posOnes.clear();
    if (j == 3) posOnes.push_back(1);
    if (j == 4) { posOnes.clear(); posOnes.push_back(10); }
    for (uint32_t ienc = 0; ienc < 4; ++ienc)
    {
      vector<uint8_t> serialBitVector;
      MemWriter< vector<uint8_t> > writer(serialBitVector);
      BuildCompressedBitVector(writer, posOnes, ienc);
      MemReader reader(serialBitVector.data(), serialBitVector.size());
      vector<uint32_t> decPosOnes = DecodeCompressedBitVector(reader);
      TEST_EQUAL(posOnes, decPosOnes, ());
    }
  }
}

UNIT_TEST(CompressedBitVector_Dense)
{
  mt19937 rng(0);
  vector<uint32_t> posOnes;
  //uint32_t prevPos = 0;
  uint32_t sum = 0;
  for (uint32_t i = 0; i < NUMS_COUNT; ++i)
  {
    uint32_t zeroesByteSize = rng() % 2 + 1;
    uint64_t zeroesRangeSize = (rng() & ((uint64_t(1) << (zeroesByteSize * 7)) - 1)) + 1;
    sum += zeroesRangeSize;
    uint32_t onesByteSize = rng() % 1 + 1;
    uint64_t onesRangeSize = (rng() & ((uint64_t(1) << (onesByteSize * 7)) - 1)) + 1;
    for (uint32_t j = 0; j < onesRangeSize; ++j) posOnes.push_back(sum + j);
    sum += onesRangeSize;
  }
  for (uint32_t j = 0; j < 5; ++j)
  {
    if (j == 1) posOnes.insert(posOnes.begin(), 1, 0);
    if (j == 2) posOnes.clear();
    if (j == 3) posOnes.push_back(1);
    if (j == 4) { posOnes.clear(); posOnes.push_back(10); }
    for (uint32_t ienc = 0; ienc < 4; ++ienc)
    {
      vector<uint8_t> serialBitVector;
      MemWriter< vector<uint8_t> > writer(serialBitVector);
      BuildCompressedBitVector(writer, posOnes, ienc);
      MemReader reader(serialBitVector.data(), serialBitVector.size());
      vector<uint32_t> decPosOnes = DecodeCompressedBitVector(reader);
      TEST_EQUAL(posOnes, decPosOnes, ());
    }
  }
}

UNIT_TEST(BitVectors_And)
{
  mt19937 rng(0);
  vector<bool> v1(NUMS_COUNT * 2, false), v2(NUMS_COUNT * 2, false);
  for (uint32_t i = 0; i < NUMS_COUNT; ++i)
  {
    v1[rng() % v1.size()] = true;
    v2[rng() % v2.size()] = true;
  }
  vector<uint32_t> posOnes1, posOnes2, andPos;
  for (uint32_t i = 0; i < v1.size(); ++i)
  {
    if (v1[i]) posOnes1.push_back(i);
    if (v2[i]) posOnes2.push_back(i);
    if (v1[i] && v2[i]) andPos.push_back(i);
  }
  vector<uint32_t> actualAndPos = BitVectorsAnd(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());
  TEST_EQUAL(andPos, actualAndPos, ());
}

UNIT_TEST(BitVectors_Or)
{
  mt19937 rng(0);
  vector<bool> v1(NUMS_COUNT * 2, false), v2(NUMS_COUNT * 2, false);
  for (uint32_t i = 0; i < NUMS_COUNT; ++i)
  {
    v1[rng() % v1.size()] = true;
    v2[rng() % v2.size()] = true;
  }
  vector<uint32_t> posOnes1, posOnes2, orPos;
  for (uint32_t i = 0; i < v1.size(); ++i)
  {
    if (v1[i]) posOnes1.push_back(i);
    if (v2[i]) posOnes2.push_back(i);
    if (v1[i] || v2[i]) orPos.push_back(i);
  }
  vector<uint32_t> actualOrPos = BitVectorsOr(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());
  TEST_EQUAL(orPos, actualOrPos, ());
}

UNIT_TEST(BitVectors_SubAnd)
{
  mt19937 rng(0);
  vector<bool> v1(NUMS_COUNT * 2, false);
  //uint64_t numV1Ones = 0;
  for (uint32_t i = 0; i < v1.size(); ++i)
    v1[i] = (rng() % 2) == 0;
  vector<uint32_t> posOnes1;
  for (uint32_t i = 0; i < v1.size(); ++i) if (v1[i]) posOnes1.push_back(i);
  vector<bool> v2(posOnes1.size(), false);
  for (uint32_t i = 0; i < v2.size(); ++i)
    v2[i] = (rng() % 2) == 0;
  vector<uint32_t> posOnes2, subandPos;
  for (uint32_t i = 0; i < v2.size(); ++i) if (v2[i]) posOnes2.push_back(i);
  for (uint32_t i = 0, j = 0; i < v1.size(); ++i)
  {
    if (v1[i])
    {
      if (v2[j]) subandPos.push_back(i);
      ++j;
    }
  }
  vector<uint32_t> actualSubandPos = BitVectorsSubAnd(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());
  TEST_EQUAL(subandPos, actualSubandPos, ());
}
