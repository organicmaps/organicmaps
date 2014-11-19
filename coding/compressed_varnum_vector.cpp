#include "arithmetic_codec.hpp"
#include "bit_streams.hpp"
#include "compressed_varnum_vector.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "varint_misc.hpp"

#include "../base/bits.hpp"
#include "../std/algorithm.hpp"
#include "../std/unique_ptr.hpp"
#include "../std/vector.hpp"

namespace {
  vector<u32> SerialFreqsToDistrTable(Reader & reader, u64 & decodeOffset, u64 cnt)
  {
    vector<u32> freqs;
    for (u64 i = 0; i < cnt; ++i) freqs.push_back(VarintDecode(reader, decodeOffset));
    return FreqsToDistrTable(freqs);
  }
}

void BuildCompressedVarnumVector(Writer & writer, NumsSourceFuncT numsSource, u64 numsCnt, bool supportSums)
{
  // Encode header.
  VarintEncode(writer, numsCnt);
  VarintEncode(writer, NUM_ELEM_PER_TABLE_ENTRY);
  VarintEncode(writer, supportSums ? 1 : 0);

  // Compute frequencies of bits sizes of all nums.
  vector<u32> sizesFreqs(65, 0);
  int32_t maxBitsSize = -1;
  for (u64 i = 0; i < numsCnt; ++i)
  {
    u64 num = numsSource(i);
    u32 bitsUsed = bits::NumUsedBits(num);
    ++sizesFreqs[bitsUsed];
    if (int32_t(bitsUsed) > maxBitsSize) maxBitsSize = bitsUsed;
  }
  sizesFreqs.resize(maxBitsSize + 1);
  VarintEncode(writer, sizesFreqs.size());
  for (u32 i = 0; i < sizesFreqs.size(); ++i) VarintEncode(writer, sizesFreqs[i]);
  
  vector<u32> distr_table = FreqsToDistrTable(sizesFreqs);

  vector<u8> encoded_table;
  u64 tableSize = numsCnt == 0 ? 1 : ((numsCnt - 1) / NUM_ELEM_PER_TABLE_ENTRY) + 2;
  u64 inum = 0, prevChunkPos = 0, encodedNumsSize = 0, prevChunkSum = 0, sum = 0;
  {
    // Encode starting table entry.
    VarintEncode(encoded_table, 0);
    if (supportSums) VarintEncode(encoded_table, 0);
  }
  for (u64 itable = 0; itable < tableSize && inum < numsCnt; ++itable)
  {
    // Encode chunk of nums (one chunk for one table entry).    
    vector<u8> encodedChunk, encodedBits;
    ArithmeticEncoder arithEncSizes(distr_table);
    {
      MemWriter< vector<u8> > encoded_bits_writer(encodedBits);
      BitSink bitsWriter(encoded_bits_writer);
      for (u64 ichunkNum = 0; ichunkNum < NUM_ELEM_PER_TABLE_ENTRY && inum < numsCnt; ++ichunkNum, ++inum)
      {
        u64 num = numsSource(inum);
        u32 bitsUsed = bits::NumUsedBits(num);
        arithEncSizes.Encode(bitsUsed);
        if (bitsUsed > 1) bitsWriter.Write(num, bitsUsed - 1);
        sum += num;
      }
    }
    vector<u8> encodedChunkSizes = arithEncSizes.Finalize();
    VarintEncode(encodedChunk, encodedChunkSizes.size());
    encodedChunk.insert(encodedChunk.end(), encodedChunkSizes.begin(), encodedChunkSizes.end());
    encodedChunk.insert(encodedChunk.end(), encodedBits.begin(), encodedBits.end());
    writer.Write(encodedChunk.data(), encodedChunk.size());
    encodedNumsSize += encodedChunk.size();

    // Encode table entry.
    VarintEncode(encoded_table, encodedNumsSize - prevChunkPos);
    if (supportSums) VarintEncode(encoded_table, sum - prevChunkSum);
    prevChunkPos = encodedNumsSize;
    prevChunkSum = sum;
  }
  writer.Write(encoded_table.data(), encoded_table.size());
  VarintEncode(writer, encoded_table.size());
}

struct CompressedVarnumVectorReader::DecodeContext
{
  unique_ptr<Reader> m_sizesArithDecReader;
  unique_ptr<ArithmeticDecoder> m_sizesArithDec;
  unique_ptr<Reader> m_numsBitsReaderReader;
  unique_ptr<BitSource> m_numsBitsReader;
  u64 m_numsLeftInChunk;
};

CompressedVarnumVectorReader::CompressedVarnumVectorReader(Reader & reader)
  : m_reader(reader), m_numsCnt(0), m_numElemPerTableEntry(0), m_supportSums(false),
    m_numsEncodedOffset(0), m_decodeCtx(0)
{
  CHECK_GREATER(reader.Size(), 0, ());
  // Decode header.
  u64 offset = 0;
  m_numsCnt = VarintDecode(m_reader, offset);
  m_numElemPerTableEntry = VarintDecode(m_reader, offset);
  m_supportSums = VarintDecode(m_reader, offset) != 0;
  vector<u32> sizesFreqs;
  u64 freqsCnt = VarintDecode(m_reader, offset);
  for (u32 i = 0; i < freqsCnt; ++i) sizesFreqs.push_back(VarintDecode(m_reader, offset));
  m_distrTable = FreqsToDistrTable(sizesFreqs);
  m_numsEncodedOffset = offset;

  // Decode jump table.  
  u64 tableSize = m_numsCnt == 0 ? 0 : ((m_numsCnt - 1) / m_numElemPerTableEntry) + 1;
  u64 tableDecodeOffset = reader.Size() - 1;
  u64 tableSizeEncodedSize = VarintDecodeReverse(reader, tableDecodeOffset);
  // Advance offset to point to the first byte of table size encoded varint.
  ++tableDecodeOffset;
  u64 tableEncodedBegin = tableDecodeOffset - tableSizeEncodedSize;
  u64 tableEncodedEnd = tableDecodeOffset;
  u64 prevPos = 0, prevSum = 0;
  for (u64 tableOffset = tableEncodedBegin; tableOffset < tableEncodedEnd;)
  {
    u64 posDiff = VarintDecode(reader, tableOffset);
    m_tablePos.push_back(prevPos + posDiff);
    prevPos += posDiff;
    if (m_supportSums)
    {
      u64 sumDiff = VarintDecode(reader, tableOffset);
      m_tableSum.push_back(prevSum + sumDiff);
      prevSum += sumDiff;
    }
  }
}

CompressedVarnumVectorReader::~CompressedVarnumVectorReader()
{
  if (m_decodeCtx) delete m_decodeCtx;
}

void CompressedVarnumVectorReader::SetDecodeContext(u64 tableEntryIndex)
{
  CHECK_LESS(tableEntryIndex, m_tablePos.size() - 1, ());
  u64 decodeOffset = m_numsEncodedOffset + m_tablePos[tableEntryIndex];
  u64 encodedSizesSize = VarintDecode(m_reader, decodeOffset);
  // Create decode context.
  if (m_decodeCtx) delete m_decodeCtx;
  m_decodeCtx = new DecodeContext;
  m_decodeCtx->m_sizesArithDecReader.reset(m_reader.CreateSubReader(decodeOffset, encodedSizesSize));
  m_decodeCtx->m_sizesArithDec.reset(new ArithmeticDecoder(*m_decodeCtx->m_sizesArithDecReader, m_distrTable));
  m_decodeCtx->m_numsBitsReaderReader.reset(m_reader.CreateSubReader(decodeOffset + encodedSizesSize, m_numsEncodedOffset + m_tablePos[tableEntryIndex + 1] - decodeOffset - encodedSizesSize));
  m_decodeCtx->m_numsBitsReader.reset(new BitSource(*m_decodeCtx->m_numsBitsReaderReader));
  m_decodeCtx->m_numsLeftInChunk = min((tableEntryIndex + 1) * m_numElemPerTableEntry, m_numsCnt) - tableEntryIndex * m_numElemPerTableEntry;
}

void CompressedVarnumVectorReader::FindByIndex(u64 index, u64 & sumBefore)
{
  CHECK_LESS(index, m_numsCnt, ());
  u64 tableEntryIndex = index / m_numElemPerTableEntry;
  u64 indexWithinRange = index % m_numElemPerTableEntry;
  
  this->SetDecodeContext(tableEntryIndex);
  
  u64 sum = 0;
  if (m_supportSums) sum = m_tableSum[tableEntryIndex];
  for (u64 i = 0; i < indexWithinRange; ++i)
  {
    u64 num = this->Read();
    if (m_supportSums) sum += num;
  }
  if (m_supportSums) sumBefore = sum;
}

u64 CompressedVarnumVectorReader::FindBySum(u64 sum, u64 & sumIncl, u64 & cntIncl)
{
  CHECK(m_supportSums, ());
  // First do binary search over select table to find the biggest
  // sum that is less than our.
  u64 l = 0, r = m_tablePos.size();
  while (r - l > 1)
  {
    u64 m = (l + r) / 2;
    if (sum > m_tableSum[m])
    {
      l = m;
    }
    else
    {
      r = m;
    }
  }
  u64 tableEntryIndex = l;
  cntIncl = tableEntryIndex * m_numElemPerTableEntry;
  
  this->SetDecodeContext(tableEntryIndex);

  sumIncl = m_tableSum[tableEntryIndex];
  u64 num = 0;
  while (sumIncl < sum && cntIncl < m_numsCnt)
  {
    num = this->Read();
    sumIncl += num;
    ++cntIncl;
    if (sumIncl >= sum) break;
  }
  return num;
}

u64 CompressedVarnumVectorReader::Read()
{
  CHECK(m_decodeCtx != 0, ());
  CHECK_GREATER(m_decodeCtx->m_numsLeftInChunk, 0, ());
  u32 bitsUsed = m_decodeCtx->m_sizesArithDec->Decode();
  if (bitsUsed == 0) return 0;
  u64 num = (u64(1) << (bitsUsed - 1)) | m_decodeCtx->m_numsBitsReader->Read(bitsUsed - 1);
  --m_decodeCtx->m_numsLeftInChunk;
  return num;
}
