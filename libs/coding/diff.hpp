#pragma once

#include "base/assert.hpp"
#include "base/base.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace diff
{
enum Operation
{
  OPERATION_COPY = 0,
  OPERATION_DELETE = 1,
  OPERATION_INSERT = 2,
};

template <class PatchWriterT, typename SizeT = uint64_t>
class PatchCoder
{
public:
  typedef SizeT size_type;

  explicit PatchCoder(PatchWriterT & patchWriter)
    : m_LastOperation(OPERATION_COPY)
    , m_LastOpCode(0)
    , m_PatchWriter(patchWriter)
  {}

  void Delete(size_type n)
  {
    if (n != 0)
      Op(OPERATION_DELETE, n);
  }

  void Copy(size_type n)
  {
    if (n != 0)
      Op(OPERATION_COPY, n);
  }

  template <typename TIter>
  void Insert(TIter it, size_type n)
  {
    if (n != 0)
    {
      Op(OPERATION_INSERT, n);
      m_PatchWriter.WriteData(it, n);
    }
  }

  void Finalize() { WriteLasOp(); }

private:
  void Op(Operation op, size_type n)
  {
    if (m_LastOperation == op)
    {
      m_LastOpCode += (n << 1);
      return;
    }
    WriteLasOp();
    m_LastOpCode = (n << 1) | ((m_LastOperation + 1) % 3 == op ? 0 : 1);
    m_LastOperation = op;
  }

  void WriteLasOp()
  {
    if (m_LastOpCode != 0)
      m_PatchWriter.WriteOperation(m_LastOpCode);
    else
      CHECK_EQUAL(m_LastOperation, OPERATION_COPY, ());  // "We were just initialized."
  }

  Operation m_LastOperation;
  size_type m_LastOpCode;
  PatchWriterT & m_PatchWriter;
};

// Find minimal patch, with no more than maxPatchSize edited values, that transforms A into B.
// Returns the length of the minimal patch, or -1 if no such patch found.
// Intermediate information is saved into tmpSink and can be used later to restore
// the resulting patch.
template <typename TSignedWord,  // Signed word, capable of storing position in text.
          class TSrcVector,      // Source data (A).
          class TDstVector,      // Destination data (B).
          class TTmpFileSink     // Sink to store temporary information.
          >
TSignedWord DiffMyersSimple(TSrcVector const & A, TDstVector const & B, TSignedWord maxPatchSize,
                            TTmpFileSink & tmpSink)
{
  ASSERT_GREATER(maxPatchSize, 0, ());
  std::vector<TSignedWord> V(2 * maxPatchSize + 1);
  for (TSignedWord d = 0; d <= maxPatchSize; ++d)
  {
    for (TSignedWord k = -d; k <= d; k += 2)
    {
      TSignedWord x;
      if (k == -d || (k != d && V[maxPatchSize + k - 1] < V[maxPatchSize + k + 1]))
        x = V[maxPatchSize + k + 1];
      else
        x = V[maxPatchSize + k - 1] + 1;
      while (x < static_cast<TSignedWord>(A.size()) && x - k < static_cast<TSignedWord>(B.size()) && A[x] == B[x - k])
        ++x;
      V[maxPatchSize + k] = x;
      if (x == static_cast<TSignedWord>(A.size()) && x - k == static_cast<TSignedWord>(B.size()))
        return d;
    }
    tmpSink.Write(&V[maxPatchSize - d], (2 * d + 1) * sizeof(TSignedWord));
  }
  return -1;
}

// Differ that just replaces old with new, with the only optimization of skipping equal values
// at the beginning and at the end.
class SimpleReplaceDiffer
{
public:
  template <typename SrcIterT, typename DstIterT, class PatchCoderT>
  void Diff(SrcIterT srcBeg, SrcIterT srcEnd, DstIterT dstBeg, DstIterT dstEnd, PatchCoderT & patchCoder)
  {
    typename PatchCoderT::size_type begCopy = 0;
    for (; srcBeg != srcEnd && dstBeg != dstEnd && *srcBeg == *dstBeg; ++srcBeg, ++dstBeg)
      ++begCopy;
    patchCoder.Copy(begCopy);
    typename PatchCoderT::size_type endCopy = 0;
    for (; srcBeg != srcEnd && dstBeg != dstEnd && *(srcEnd - 1) == *(dstEnd - 1); --srcEnd, --dstEnd)
      ++endCopy;
    patchCoder.Delete(srcEnd - srcBeg);
    patchCoder.Insert(dstBeg, dstEnd - dstBeg);
    patchCoder.Copy(endCopy);
  }
};

// Given FineGrainedDiff and rolling Hasher, DiffWithRollingHash splits the source sequence
// into chunks of size m_BlockSize, finds equal chunks in the destination sequence, using rolling
// hash to find good candidates, writes info about equal chunks into patchCoder and for everything
// between equal chunks, calls FineGrainedDiff::Diff().
template <class FineGrainedDiffT, class HasherT,
          class HashPosMultiMapT = std::unordered_multimap<typename HasherT::hash_type, uint64_t>>
class RollingHashDiffer
{
public:
  explicit RollingHashDiffer(size_t blockSize, FineGrainedDiffT const & fineGrainedDiff = FineGrainedDiffT())
    : m_FineGrainedDiff(fineGrainedDiff)
    , m_BlockSize(blockSize)
  {}

  template <typename SrcIterT, typename DstIterT, class PatchCoderT>
  void Diff(SrcIterT const srcBeg, SrcIterT const srcEnd, DstIterT const dstBeg, DstIterT const dstEnd,
            PatchCoderT & patchCoder)
  {
    if (srcEnd - srcBeg < static_cast<decltype(srcEnd - srcBeg)>(m_BlockSize) ||
        dstEnd - dstBeg < static_cast<decltype(dstEnd - dstBeg)>(m_BlockSize))
    {
      m_FineGrainedDiff.Diff(srcBeg, srcEnd, dstBeg, dstEnd, patchCoder);
      return;
    }
    HasherT hasher;
    HashPosMultiMapT srcHashes;
    for (SrcIterT src = srcBeg; srcEnd - src >= static_cast<decltype(srcEnd - src)>(m_BlockSize); src += m_BlockSize)
      srcHashes.insert(HashPosMultiMapValue(hasher.Init(src, m_BlockSize), src - srcBeg));
    SrcIterT srcLastDiff = srcBeg;
    DstIterT dst = dstBeg, dstNext = dstBeg + m_BlockSize, dstLastDiff = dstBeg;
    hash_type h = hasher.Init(dst, m_BlockSize);
    while (dstNext != dstEnd)
    {
      std::pair<HashPosMultiMapIterator, HashPosMultiMapIterator> iters = srcHashes.equal_range(h);
      if (iters.first != iters.second)
      {
        pos_type const srcLastDiffPos = srcLastDiff - srcBeg;
        HashPosMultiMapIterator it = srcHashes.end();
        for (HashPosMultiMapIterator i = iters.first; i != iters.second; ++i)
          if (i->second >= srcLastDiffPos && (it == srcHashes.end() || i->second < it->second))
            it = i;
        if (it != srcHashes.end() && std::equal(srcBeg + it->second, srcBeg + it->second + m_BlockSize, dst))
        {
          pos_type srcBlockEqualPos = it->second;
          m_FineGrainedDiff.Diff(srcLastDiff, srcBeg + srcBlockEqualPos, dstLastDiff, dst, patchCoder);
          patchCoder.Copy(m_BlockSize);
          srcLastDiff = srcBeg + srcBlockEqualPos + m_BlockSize;
          dst = dstLastDiff = dstNext;
          if (dstEnd - dstNext < static_cast<decltype(dstEnd - dstNext)>(m_BlockSize))
            break;
          dstNext = dst + m_BlockSize;
          h = hasher.Init(dst, m_BlockSize);
          continue;
        }
      }
      h = hasher.Scroll(*(dst++), *(dstNext++));
    }
    if (srcLastDiff != srcEnd || dstLastDiff != dstEnd)
      m_FineGrainedDiff.Diff(srcLastDiff, srcEnd, dstLastDiff, dstEnd, patchCoder);
  }

private:
  typedef typename HasherT::hash_type hash_type;
  typedef typename HashPosMultiMapT::value_type::second_type pos_type;
  typedef typename HashPosMultiMapT::const_iterator HashPosMultiMapIterator;
  typedef typename HashPosMultiMapT::value_type HashPosMultiMapValue;

  FineGrainedDiffT m_FineGrainedDiff;
  HasherT m_Hasher;
  size_t m_BlockSize;
};
}  // namespace diff
