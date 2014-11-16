#include "../compressed_bit_vector.hpp"
#include "../reader.hpp"
#include "../writer.hpp"

#include "../../testing/testing.hpp"
#include "../../base/pseudo_random.hpp"

u32 const NUMS_COUNT = 12345;

namespace
{
  u64 GetRand64()
  {
    static PseudoRNG32 g_rng;
    u64 result = g_rng.Generate();
    result ^= u64(g_rng.Generate()) << 32;
    return result;
  }
}

UNIT_TEST(CompressedBitVector_Sparse)
{
  vector<u32> posOnes;
  u32 sum = 0;
  for (u32 i = 0; i < NUMS_COUNT; ++i)
  {
    u32 byteSize = GetRand64() % 2 + 1;
    u64 num = GetRand64() & ((u64(1) << (byteSize * 7)) - 1);
    if (num == 0) num = 1;
    sum += num;
    posOnes.push_back(sum);
  }
  for (u32 j = 0; j < 5; ++j)
  {
    if (j == 1) posOnes.insert(posOnes.begin(), 1, 0);
    if (j == 2) posOnes.clear();
    if (j == 3) posOnes.push_back(1);
    if (j == 4) { posOnes.clear(); posOnes.push_back(10); }
    for (u32 ienc = 0; ienc < 4; ++ienc)
    {
      vector<u8> serialBitVector;
      MemWriter< vector<u8> > writer(serialBitVector);
      BuildCompressedBitVector(writer, posOnes, ienc);
      MemReader reader(serialBitVector.data(), serialBitVector.size());
      vector<u32> decPosOnes = DecodeCompressedBitVector(reader);
      TEST_EQUAL(posOnes, decPosOnes, ());
    }
  }
}

UNIT_TEST(CompressedBitVector_Dense)
{
  vector<u32> posOnes;
  u32 prevPos = 0;
  u32 sum = 0;
  for (u32 i = 0; i < NUMS_COUNT; ++i)
  {
    u32 zeroesByteSize = GetRand64() % 2 + 1;
    u64 zeroesRangeSize = (GetRand64() & ((u64(1) << (zeroesByteSize * 7)) - 1)) + 1;
    sum += zeroesRangeSize;
    u32 onesByteSize = GetRand64() % 1 + 1;
    u64 onesRangeSize = (GetRand64() & ((u64(1) << (onesByteSize * 7)) - 1)) + 1;
    for (u32 j = 0; j < onesRangeSize; ++j) posOnes.push_back(sum + j);
    sum += onesRangeSize;
  }
  for (u32 j = 0; j < 5; ++j)
  {
    if (j == 1) posOnes.insert(posOnes.begin(), 1, 0);
    if (j == 2) posOnes.clear();
    if (j == 3) posOnes.push_back(1);
    if (j == 4) { posOnes.clear(); posOnes.push_back(10); }
    for (u32 ienc = 0; ienc < 4; ++ienc)
    {
      vector<u8> serialBitVector;
      MemWriter< vector<u8> > writer(serialBitVector);
      BuildCompressedBitVector(writer, posOnes, ienc);
      MemReader reader(serialBitVector.data(), serialBitVector.size());
      vector<u32> decPosOnes = DecodeCompressedBitVector(reader);
      TEST_EQUAL(posOnes, decPosOnes, ());
    }
  }
}

UNIT_TEST(BitVectors_And)
{
  vector<bool> v1(NUMS_COUNT * 2, false), v2(NUMS_COUNT * 2, false);
  for (u32 i = 0; i < NUMS_COUNT; ++i)
  {
    v1[GetRand64() % v1.size()] = true;
    v2[GetRand64() % v2.size()] = true;
  }
  vector<u32> posOnes1, posOnes2, andPos;
  for (u32 i = 0; i < v1.size(); ++i)
  {
    if (v1[i]) posOnes1.push_back(i);
    if (v2[i]) posOnes2.push_back(i);
    if (v1[i] && v2[i]) andPos.push_back(i);
  }
  vector<u32> actualAndPos = BitVectorsAnd(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());
  TEST_EQUAL(andPos, actualAndPos, ());
}

UNIT_TEST(BitVectors_Or)
{
  vector<bool> v1(NUMS_COUNT * 2, false), v2(NUMS_COUNT * 2, false);
  for (u32 i = 0; i < NUMS_COUNT; ++i)
  {
    v1[GetRand64() % v1.size()] = true;
    v2[GetRand64() % v2.size()] = true;
  }
  vector<u32> posOnes1, posOnes2, orPos;
  for (u32 i = 0; i < v1.size(); ++i)
  {
    if (v1[i]) posOnes1.push_back(i);
    if (v2[i]) posOnes2.push_back(i);
    if (v1[i] || v2[i]) orPos.push_back(i);
  }
  vector<u32> actualOrPos = BitVectorsOr(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());
  TEST_EQUAL(orPos, actualOrPos, ());
}

UNIT_TEST(BitVectors_SubAnd)
{
  vector<bool> v1(NUMS_COUNT * 2, false);
  u64 numV1Ones = 0;
  for (u32 i = 0; i < v1.size(); ++i) v1[i] = (GetRand64() % 2) == 0;
  vector<u32> posOnes1;
  for (u32 i = 0; i < v1.size(); ++i) if (v1[i]) posOnes1.push_back(i);
  vector<bool> v2(posOnes1.size(), false);
  for (u32 i = 0; i < v2.size(); ++i) v2[i] = (GetRand64() % 2) == 0;
  vector<u32> posOnes2, subandPos;
  for (u32 i = 0; i < v2.size(); ++i) if (v2[i]) posOnes2.push_back(i);
  for (u32 i = 0, j = 0; i < v1.size(); ++i)
  {
    if (v1[i])
    {
      if (v2[j]) subandPos.push_back(i);
      ++j;
    }
  }
  vector<u32> actualSubandPos = BitVectorsSubAnd(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());
  TEST_EQUAL(subandPos, actualSubandPos, ());
}
