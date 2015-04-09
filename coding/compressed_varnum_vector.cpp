#include "coding/compressed_varnum_vector.hpp"

/*
#include "coding/arithmetic_codec.hpp"
#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"
#include "coding/varint_misc.hpp"

#include "base/bits.hpp"
#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

//namespace {
//  vector<uint32_t> SerialFreqsToDistrTable(Reader & reader, uint64_t & decodeOffset, uint64_t cnt)
//  {
//    vector<uint32_t> freqs;
//    for (uint64_t i = 0; i < cnt; ++i) freqs.push_back(VarintDecode(reader, decodeOffset));
//    return FreqsToDistrTable(freqs);
//  }
//}

void BuildCompressedVarnumVector(Writer & writer, NumsSourceFuncT numsSource, uint64_t numsCnt, bool supportSums)
{
  // Encode header.
  VarintEncode(writer, numsCnt);
  VarintEncode(writer, NUM_ELEM_PER_TABLE_ENTRY);
  VarintEncode(writer, supportSums ? 1 : 0);

  // Compute frequencies of bits sizes of all nums.
  vector<uint32_t> sizesFreqs(65, 0);
  int32_t maxBitsSize = -1;
  for (uint64_t i = 0; i < numsCnt; ++i)
  {
    uint64_t num = numsSource(i);
    uint32_t bitsUsed = bits::NumUsedBits(num);
    ++sizesFreqs[bitsUsed];
    if (int32_t(bitsUsed) > maxBitsSize) maxBitsSize = bitsUsed;
  }
  sizesFreqs.resize(maxBitsSize + 1);
  VarintEncode(writer, sizesFreqs.size());
  for (uint32_t i = 0; i < sizesFreqs.size(); ++i) VarintEncode(writer, sizesFreqs[i]);
  
  vector<uint32_t> distr_table = FreqsToDistrTable(sizesFreqs);

  vector<uint8_t> encoded_table;
  uint64_t tableSize = numsCnt == 0 ? 1 : ((numsCnt - 1) / NUM_ELEM_PER_TABLE_ENTRY) + 2;
  uint64_t inum = 0, prevChunkPos = 0, encodedNumsSize = 0, prevChunkSum = 0, sum = 0;
  {
    // Encode starting table entry.
    VarintEncode(encoded_table, 0);
    if (supportSums) VarintEncode(encoded_table, 0);
  }
  for (uint64_t itable = 0; itable < tableSize && inum < numsCnt; ++itable)
  {
    // Encode chunk of nums (one chunk for one table entry).    
    vector<uint8_t> encodedChunk, encodedBits;
    ArithmeticEncoder arithEncSizes(distr_table);
    {
      MemWriter< vector<uint8_t> > encoded_bits_writer(encodedBits);
      BitSink bitsWriter(encoded_bits_writer);
      for (uint64_t ichunkNum = 0; ichunkNum < NUM_ELEM_PER_TABLE_ENTRY && inum < numsCnt; ++ichunkNum, ++inum)
      {
        uint64_t num = numsSource(inum);
        uint32_t bitsUsed = bits::NumUsedBits(num);
        arithEncSizes.Encode(bitsUsed);
        if (bitsUsed > 1) bitsWriter.Write(num, bitsUsed - 1);
        sum += num;
      }
    }
    vector<uint8_t> encodedChunkSizes = arithEncSizes.Finalize();
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
  uint64_t m_numsLeftInChunk;
};

CompressedVarnumVectorReader::CompressedVarnumVectorReader(Reader & reader)
  : m_reader(reader), m_numsCnt(0), m_numElemPerTableEntry(0), m_supportSums(false),
    m_numsEncodedOffset(0), m_decodeCtx(0)
{
  CHECK_GREATER(reader.Size(), 0, ());
  // Decode header.
  uint64_t offset = 0;
  m_numsCnt = VarintDecode(m_reader, offset);
  m_numElemPerTableEntry = VarintDecode(m_reader, offset);
  m_supportSums = VarintDecode(m_reader, offset) != 0;
  vector<uint32_t> sizesFreqs;
  uint64_t freqsCnt = VarintDecode(m_reader, offset);
  for (uint32_t i = 0; i < freqsCnt; ++i) sizesFreqs.push_back(VarintDecode(m_reader, offset));
  m_distrTable = FreqsToDistrTable(sizesFreqs);
  m_numsEncodedOffset = offset;

  // Decode jump table.  
  //uint64_t tableSize = m_numsCnt == 0 ? 0 : ((m_numsCnt - 1) / m_numElemPerTableEntry) + 1;
  uint64_t tableDecodeOffset = reader.Size() - 1;
  uint64_t tableSizeEncodedSize = VarintDecodeReverse(reader, tableDecodeOffset);
  // Advance offset to point to the first byte of table size encoded varint.
  ++tableDecodeOffset;
  uint64_t tableEncodedBegin = tableDecodeOffset - tableSizeEncodedSize;
  uint64_t tableEncodedEnd = tableDecodeOffset;
  uint64_t prevPos = 0, prevSum = 0;
  for (uint64_t tableOffset = tableEncodedBegin; tableOffset < tableEncodedEnd;)
  {
    uint64_t posDiff = VarintDecode(reader, tableOffset);
    m_tablePos.push_back(prevPos + posDiff);
    prevPos += posDiff;
    if (m_supportSums)
    {
      uint64_t sumDiff = VarintDecode(reader, tableOffset);
      m_tableSum.push_back(prevSum + sumDiff);
      prevSum += sumDiff;
    }
  }
}

CompressedVarnumVectorReader::~CompressedVarnumVectorReader()
{
  if (m_decodeCtx) delete m_decodeCtx;
}

void CompressedVarnumVectorReader::SetDecodeContext(uint64_t tableEntryIndex)
{
  CHECK_LESS(tableEntryIndex, m_tablePos.size() - 1, ());
  uint64_t decodeOffset = m_numsEncodedOffset + m_tablePos[tableEntryIndex];
  uint64_t encodedSizesSize = VarintDecode(m_reader, decodeOffset);
  // Create decode context.
  if (m_decodeCtx) delete m_decodeCtx;
  m_decodeCtx = new DecodeContext;
  m_decodeCtx->m_sizesArithDecReader.reset(m_reader.CreateSubReader(decodeOffset, encodedSizesSize));
  m_decodeCtx->m_sizesArithDec.reset(new ArithmeticDecoder(*m_decodeCtx->m_sizesArithDecReader, m_distrTable));
  m_decodeCtx->m_numsBitsReaderReader.reset(m_reader.CreateSubReader(decodeOffset + encodedSizesSize, m_numsEncodedOffset + m_tablePos[tableEntryIndex + 1] - decodeOffset - encodedSizesSize));
  m_decodeCtx->m_numsBitsReader.reset(new BitSource(*m_decodeCtx->m_numsBitsReaderReader));
  m_decodeCtx->m_numsLeftInChunk = min((tableEntryIndex + 1) * m_numElemPerTableEntry, m_numsCnt) - tableEntryIndex * m_numElemPerTableEntry;
}

void CompressedVarnumVectorReader::FindByIndex(uint64_t index, uint64_t & sumBefore)
{
  CHECK_LESS(index, m_numsCnt, ());
  uint64_t tableEntryIndex = index / m_numElemPerTableEntry;
  uint64_t indexWithinRange = index % m_numElemPerTableEntry;
  
  this->SetDecodeContext(tableEntryIndex);
  
  uint64_t sum = 0;
  if (m_supportSums) sum = m_tableSum[tableEntryIndex];
  for (uint64_t i = 0; i < indexWithinRange; ++i)
  {
    uint64_t num = this->Read();
    if (m_supportSums) sum += num;
  }
  if (m_supportSums) sumBefore = sum;
}

uint64_t CompressedVarnumVectorReader::FindBySum(uint64_t sum, uint64_t & sumIncl, uint64_t & cntIncl)
{
  CHECK(m_supportSums, ());
  // First do binary search over select table to find the biggest
  // sum that is less than our.
  uint64_t l = 0, r = m_tablePos.size();
  while (r - l > 1)
  {
    uint64_t m = (l + r) / 2;
    if (sum > m_tableSum[m])
    {
      l = m;
    }
    else
    {
      r = m;
    }
  }
  uint64_t tableEntryIndex = l;
  cntIncl = tableEntryIndex * m_numElemPerTableEntry;
  
  this->SetDecodeContext(tableEntryIndex);

  sumIncl = m_tableSum[tableEntryIndex];
  uint64_t num = 0;
  while (sumIncl < sum && cntIncl < m_numsCnt)
  {
    num = this->Read();
    sumIncl += num;
    ++cntIncl;
    if (sumIncl >= sum) break;
  }
  return num;
}

uint64_t CompressedVarnumVectorReader::Read()
{
  CHECK(m_decodeCtx != 0, ());
  CHECK_GREATER(m_decodeCtx->m_numsLeftInChunk, 0, ());
  uint32_t bitsUsed = m_decodeCtx->m_sizesArithDec->Decode();
  if (bitsUsed == 0) return 0;
  uint64_t num = (uint64_t(1) << (bitsUsed - 1)) | m_decodeCtx->m_numsBitsReader->Read(bitsUsed - 1);
  --m_decodeCtx->m_numsLeftInChunk;
  return num;
}
*/
