#include "../../testing/testing.hpp"
#include "../dd_vector.hpp"
#include "../dd_bit_vector.hpp"
#include "../dd_bit_rank_directory.hpp"
#include "../bit_vector_builder.hpp"
#include "../byte_stream.hpp"
#include "../reader.hpp"
#include "../../base/base.hpp"
#include "../../base/macros.hpp"
#include "../../std/cstdlib.hpp"

#include "../../base/start_mem_debug.hpp"

namespace
{
  template <typename TWord>
  void TestBitVector(unsigned char const * bits, size_t N)
  {
    typedef PushBackByteSink<vector<char> > SinkType;
    vector<char> data;
    SinkType sink(data);
    BuildMMBitVector<TWord>(sink, &bits[0], &bits[N]);

    DDBitVector<DDVector<TWord, MemReader> > bitVectorDD;
    MemReader reader(&data[0], data.size());
    DDParseInfo<MemReader> info(reader, true);
    bitVectorDD.Parse(info);
    for (size_t i = 0; i < N; ++i)
      TEST_EQUAL(bitVectorDD[i], bits[i] != 0, (i));
  }

  void TestBitVector32Rank(unsigned char const * bits, size_t N, size_t logChunkSize)
  {
    typedef PushBackByteSink<vector<char> > SinkType;
    vector<char> data;
    SinkType sink(data);
    BuildMMBitVector<uint32_t>(sink, &bits[0], &bits[N]);
    BuildMMBitVector32RankDirectory(sink, &bits[0], &bits[N], logChunkSize);

    DDBitRankDirectory<DDBitVector<DDVector<uint32_t, MemReader> > > rankDirectoryDD;
    MemReader reader(&data[0], data.size());
    {
      DDParseInfo<MemReader> info(reader, true);
      rankDirectoryDD.Parse(info);
    }

    uint32_t rank1 = 0;
    for (size_t i = 0; i < N; ++i)
    {
      if (rankDirectoryDD[i])
      {
        ++rank1;
        TEST_EQUAL(rankDirectoryDD.BitVector().Select1FromWord(0, rank1), i,
                   (rank1, N, logChunkSize));
        TEST_EQUAL(rankDirectoryDD.Select1(rank1), i, (rank1, N, logChunkSize));
      }
      TEST_EQUAL(rankDirectoryDD.Rank1(i), rank1, (i, N, logChunkSize));
      TEST_EQUAL(rankDirectoryDD.Rank0(i), i + 1 - rank1, (i, N, logChunkSize));
    }
  }

  unsigned char const simpleBits[] = {
    1,1,0,1,0,0,0,1,
    1,1,0,0,0,0,0,1,
    1,1,0,0,1,1,1,0,
    0,1,1,1,0,0,1,1,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,1,0,0,1,0,0,
    0,0,0,1,0,0,0,1 };
  static size_t const simpleBitsSize = ARRAY_SIZE(simpleBits);
}

UNIT_TEST(BitVector8Simple)
{
  TestBitVector<uint8_t>(simpleBits, simpleBitsSize);
}

UNIT_TEST(BitVector16Simple)
{
  TestBitVector<uint16_t>(simpleBits, simpleBitsSize);
}

UNIT_TEST(BitVector32Simple)
{
  TestBitVector<uint32_t>(simpleBits, simpleBitsSize);
}

UNIT_TEST(BitVector64Simple)
{
  TestBitVector<uint64_t>(simpleBits, simpleBitsSize);
}

UNIT_TEST(BitVector32RankSimple)
{
  for (size_t chunkSize = 1; chunkSize <= 7; ++chunkSize)
    TestBitVector32Rank(simpleBits, simpleBitsSize, chunkSize);
}

UNIT_TEST(BitVector32RankChunkSizePlus1All0)
{
  vector<unsigned char> bits(129, 0);
  TestBitVector<uint32_t>(&bits[0], bits.size());
  TestBitVector32Rank(&bits[0], bits.size(), 2);
}

UNIT_TEST(BitVector32RankChunkSizePlus1All1)
{
  vector<unsigned char> bits(129, 1);
  TestBitVector<uint32_t>(&bits[0], bits.size());
  TestBitVector32Rank(&bits[0], bits.size(), 2);
}

UNIT_TEST(BitVector32Empty)
{
  TestBitVector<uint8_t>(NULL, 0);
  TestBitVector<uint16_t>(NULL, 0);
  TestBitVector<uint32_t>(NULL, 0);
  TestBitVector<uint64_t>(NULL, 0);
  // TODO: When uncommented, there is an error:
  // malloc: *** error for object 0x12c2e: pointer being freed was not allocated
  // TestBitVector32Rank(NULL, 0, 2);
}

UNIT_TEST(BitVector32RankRandom)
{
  // TODO: When l == 0, there is an error:
  // malloc: *** error for object 0x12c2e: pointer being freed was not allocated
  // *** set a breakpoint in malloc_error_break to debug
  for (size_t l = 1; l <= 150; ++l)
  {
    vector<unsigned char> bits(l);
    for (size_t i = 0; i < bits.size(); ++i)
      bits[i] = (rand() & 1);
    unsigned char * p = bits.empty() ? NULL : &bits[0];
    TestBitVector<uint32_t>(p, bits.size());
    TestBitVector32Rank(p, bits.size(), 1);
    TestBitVector32Rank(p, bits.size(), 2);
    TestBitVector32Rank(p, bits.size(), 3);
  }
}

UNIT_TEST(BitVector32RankRandomPow2)
{
  uint32_t values[] = {16, 32, 64, 128, 256, 1024, 2048, 4096};
  for (size_t j = 0; j < ARRAY_SIZE(values); ++j)
  {
    for (size_t l = values[j] - 2; l <= values[j] + 2; ++l)
    {
      vector<unsigned char> bits(l);
      for (size_t i = 0; i < bits.size(); ++i)
        bits[i] = (rand() & 1);
      TestBitVector<uint32_t>(&bits[0], bits.size());
      TestBitVector32Rank(&bits[0], bits.size(), 1);
      TestBitVector32Rank(&bits[0], bits.size(), 2);
      TestBitVector32Rank(&bits[0], bits.size(), 3);
    }
  }
}

UNIT_TEST(BitVector32RankRandomLargeVector)
{
  size_t const l = 9000;
  vector<unsigned char> bits(l);
  for (size_t i = 0; i < bits.size(); ++i)
    bits[i] = (rand() & 1);
  unsigned char * p = &bits[0];
  TestBitVector<uint32_t>(p, bits.size());
  TestBitVector32Rank(p, bits.size(), 1);
  TestBitVector32Rank(p, bits.size(), 2);
  TestBitVector32Rank(p, bits.size(), 3);
}

// TODO: Test large bitVector.
// TODO: Test max size bitVector.
// TODO: Test wrong data size for BitVector and BitVector32RankDirectory.
#include "../../base/stop_mem_debug.hpp"
