#pragma once
#include "varint.hpp"
#include "write_to_sink.hpp"
#include "../base/base.hpp"
#include "../std/iterator.hpp"

// TWord - word type, uint32_t or uint64_t.
// TSink - where to write.
// TIter - iterator to bool.
template <typename TWord, typename TSink, typename TIter>
void BuildMMBitVector(TSink & sink, TIter beg, TIter end, bool bWriteSize = true, size_t size = -1)
{
  if (size == size_t(-1))
    size = distance(beg, end);
  CHECK(static_cast<TWord>(size) == size, ("Vector is more than word size.", size));
  if (bWriteSize)
    WriteVarUint(sink, size);

  int bitInWord = 0;
  TWord word = 0;
  for (; beg != end; ++beg)
  {
    if (*beg)
      word |= (TWord(1) << bitInWord);
    if (++bitInWord == 8 * sizeof(TWord))
    {
      WriteToSink(sink, word);
      bitInWord = 0;
      word = 0;
    }
  }
  if (bitInWord != 0)
    WriteToSink(sink, word);
}

// TSink - where to write.
// TIter - iterator to bool.
// TODO: optimize logChunkSize default value.
template <typename TSink, typename TIter>
void BuildMMBitVector32RankDirectory(TSink & sink, TIter beg, TIter end, uint32_t logChunkSize = 5)
{
  WriteVarUint(sink, logChunkSize);
  uint32_t rank1 = 0;
  for (uint32_t i = 0; beg != end; ++beg,++i)
  {
    if ((i & ((1 << (logChunkSize + 5)) - 1)) == 0 && i != 0)
      WriteToSink(sink, rank1);
    if (*beg)
      ++rank1;
  }
  WriteToSink(sink, rank1);
}
