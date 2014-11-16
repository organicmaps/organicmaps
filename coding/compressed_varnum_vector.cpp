#include "arithmetic_codec.hpp"
#include "compressed_varnum_vector.hpp"
#include "reader.hpp"
#include "writer.hpp"

#include "../std/unique_ptr.hpp"
#include "../std/vector.hpp"

namespace {
  void VarintEncode(vector<u8> & dst, u64 n)
  {
    if (n == 0)
    {
      dst.push_back(0);
    }
    else
    {
      while (n != 0)
      {
        u8 b = n & 0x7F;
        n >>= 7;
        b |= n == 0 ? 0 : 0x80;
        dst.push_back(b);
      }
    }
  }
  void VarintEncode(Writer & writer, u64 n)
  {
    if (n == 0)
    {
      writer.Write(&n, 1);
    }
    else
    {
      while (n != 0)
      {
        u8 b = n & 0x7F;
        n >>= 7;
        b |= n == 0 ? 0 : 0x80;
        writer.Write(&b, 1);
      }
    }
  }
  u64 VarintDecode(void * src, u64 & offset)
  {
    u64 n = 0;
    int shift = 0;
    while (1)
    {
      u8 b = *(((u8*)src) + offset);
      CHECK_LESS_OR_EQUAL(shift, 56, ());
      n |= u64(b & 0x7F) << shift;
      ++offset;
      if ((b & 0x80) == 0) break;
      shift += 7;
    }
    return n;
  }
  u64 VarintDecode(Reader & reader, u64 & offset)
  {
    u64 n = 0;
    int shift = 0;
    while (1)
    {
      u8 b = 0;
      reader.Read(offset, &b, 1);
      CHECK_LESS_OR_EQUAL(shift, 56, ());
      n |= u64(b & 0x7F) << shift;
      ++offset;
      if ((b & 0x80) == 0) break;
      shift += 7;
    }
    return n;
  }
  u64 VarintDecodeReverse(Reader & reader, u64 & offset)
  {
    u8 b = 0;
    do
    {
      --offset;
      reader.Read(offset, &b, 1);
    }
    while ((b & 0x80) != 0);
    ++offset;
    u64 beginOffset = offset;
    u64 num = VarintDecode(reader, offset);
    offset = beginOffset;
    return num;
  }

  inline u32 NumUsedBits(u64 n)
  {
    u32 result = 0;
    while (n != 0)
    {
      ++result;
      n >>= 1;
    }
    return result;
  }
  vector<u32> SerialFreqsToDistrTable(Reader & reader, u64 & decodeOffset, u64 cnt)
  {
    vector<u32> freqs;
    for (u64 i = 0; i < cnt; ++i) freqs.push_back(VarintDecode(reader, decodeOffset));
    return FreqsToDistrTable(freqs);
  }
  
  u64 Max(u64 a, u64 b) { return a > b ? a : b; }
  u64 Min(u64 a, u64 b) { return a < b ? a : b; }
}

class BitWriter
{
public:
  BitWriter(Writer & writer)
    : m_writer(writer), m_lastByte(0), m_size(0) {}
  ~BitWriter() { if (m_size % 8 > 0) m_writer.Write(&m_lastByte, 1); }
  u64 NumBitsWritten() const { return m_size; }
  void Write(u64 bits, u32 writeSize)
  {
    if (writeSize == 0) return;
    m_totalBits += writeSize;
    u32 remSize = m_size % 8;
    CHECK_LESS_OR_EQUAL(writeSize, 64 - remSize, ());
    if (remSize > 0)
    {
      bits <<= remSize;
      bits |= m_lastByte;
      writeSize += remSize;
      m_size -= remSize;
    }
    u32 writeBytesSize = writeSize / 8;
    m_writer.Write(&bits, writeBytesSize);
    m_lastByte = (bits >> (writeBytesSize * 8)) & ((1 << (writeSize % 8)) - 1);
    m_size += writeSize;
  }
private:
  Writer & m_writer;
  u8 m_lastByte;
  u64 m_size;
  u64 m_totalBits;
};

class BitReader
{
public:
  BitReader(Reader & reader)
    : m_reader(reader), m_serialCur(0), m_serialEnd(reader.Size()),
      m_bits(0), m_bitsSize(0), m_totalBitsRead(0) {}
  u64 NumBitsRead() const { return m_totalBitsRead; }
  u64 Read(u32 readSize)
  {
    m_totalBitsRead += readSize;
    if (readSize == 0) return 0;
    CHECK_LESS_OR_EQUAL(readSize, 64, ());
    // First read, sets bits that are in the m_bits buffer.
    u32 firstReadSize = readSize <= m_bitsSize ? readSize : m_bitsSize;
    u64 result = m_bits & (~u64(0) >> (64 - firstReadSize));
    m_bits >>= firstReadSize;
    m_bitsSize -= firstReadSize;
    readSize -= firstReadSize;
    // Second read, does an extra read using m_reader.
    if (readSize > 0)
    {
      u32 read_byte_size = m_serialCur + sizeof(m_bits) <= m_serialEnd ? sizeof(m_bits) : m_serialEnd - m_serialCur;
      m_reader.Read(m_serialCur, &m_bits, read_byte_size);
      m_serialCur += read_byte_size;
      m_bitsSize += read_byte_size * 8;
      if (readSize > m_bitsSize) CHECK_LESS_OR_EQUAL(readSize, m_bitsSize, ());
      result |= (m_bits & (~u64(0) >> (64 - readSize))) << firstReadSize;
      m_bits >>= readSize;
      m_bitsSize -= readSize;
      readSize = 0;
    }
    return result;
  }
private:
  Reader & m_reader;
  u64 m_serialCur;
  u64 m_serialEnd;
  u64 m_bits;
  u32 m_bitsSize;
  u64 m_totalBitsRead;
};

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
    u32 bitsUsed = NumUsedBits(num);
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
      BitWriter bitsWriter(encoded_bits_writer);
      for (u64 ichunkNum = 0; ichunkNum < NUM_ELEM_PER_TABLE_ENTRY && inum < numsCnt; ++ichunkNum, ++inum)
      {
        u64 num = numsSource(inum);
        u32 bitsUsed = NumUsedBits(num);
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
  unique_ptr<BitReader> m_numsBitsReader;
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
  m_decodeCtx->m_numsBitsReader.reset(new BitReader(*m_decodeCtx->m_numsBitsReaderReader));
  m_decodeCtx->m_numsLeftInChunk = Min((tableEntryIndex + 1) * m_numElemPerTableEntry, m_numsCnt) - tableEntryIndex * m_numElemPerTableEntry;
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
