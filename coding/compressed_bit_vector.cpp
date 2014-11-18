#include "compressed_bit_vector.hpp"

#include "arithmetic_codec.hpp"
#include "reader.hpp"
#include "writer.hpp"

#include "../base/assert.hpp"
#include "../base/bits.hpp"

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

  vector<u32> SerialFreqsToDistrTable(Reader & reader, u64 & decodeOffset, u64 cnt)
  {
    vector<u32> freqs;
    for (u64 i = 0; i < cnt; ++i) freqs.push_back(VarintDecode(reader, decodeOffset));
    return FreqsToDistrTable(freqs);
  }
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

class BitReader {
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
      u32 readByteSize = m_serialCur + sizeof(m_bits) <= m_serialEnd ? sizeof(m_bits) : m_serialEnd - m_serialCur;
      m_reader.Read(m_serialCur, &m_bits, readByteSize);
      m_serialCur += readByteSize;
      m_bitsSize += readByteSize * 8;
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

void BuildCompressedBitVector(Writer & writer, vector<u32> const & posOnes, int chosenEncType)
{
  u32 const BLOCK_SIZE = 7;
  // First stage of compression is analysis run through data ones.
  u64 numBytesDiffsEncVint = 0, numBytesRangesEncVint = 0, numBitsDiffsEncArith = 0, numBitsRangesEncArith = 0;
  int64_t prevOnePos = -1;
  u64 onesRangeLen = 0;
  vector<u32> diffsSizesFreqs(65, 0), ranges0SizesFreqs(65, 0), ranges1SizesFreqs(65, 0);
  for (u32 i = 0; i < posOnes.size(); ++i)
  {
    CHECK_LESS(prevOnePos, posOnes[i], ());
    // Accumulate size of diff encoding.
    u64 diff = posOnes[i] - prevOnePos;
    u32 diffBitsize = bits::NumUsedBits(diff - 1);
    numBytesDiffsEncVint += (diffBitsize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    numBitsDiffsEncArith += diffBitsize > 0 ? diffBitsize - 1 : 0;
    ++diffsSizesFreqs[diffBitsize];
    // Accumulate sizes of ranges encoding.
    if (posOnes[i] - prevOnePos > 1)
    {
      if (onesRangeLen > 0)
      {
        // Accumulate size of ones-range encoding.
        u32 onesRangeLenBitsize = bits::NumUsedBits(onesRangeLen - 1);
        numBytesRangesEncVint += (onesRangeLenBitsize + BLOCK_SIZE - 1) / BLOCK_SIZE;
        numBitsRangesEncArith += onesRangeLenBitsize > 0 ? onesRangeLenBitsize - 1 : 0;
        ++ranges1SizesFreqs[onesRangeLenBitsize];
        onesRangeLen = 0;
      }
      // Accumulate size of zeros-range encoding.
      u32 zeros_range_len_bitsize = bits::NumUsedBits(posOnes[i] - prevOnePos - 2);
      numBytesRangesEncVint += (zeros_range_len_bitsize + BLOCK_SIZE - 1) / BLOCK_SIZE;
      numBitsRangesEncArith += zeros_range_len_bitsize > 0 ? zeros_range_len_bitsize - 1 : 0;
      ++ranges0SizesFreqs[zeros_range_len_bitsize];
    }
    ++onesRangeLen;
    prevOnePos = posOnes[i];
  }
  // Accumulate size of remaining ones-range encoding.
  if (onesRangeLen > 0)
  {
    u32 onesRangeLenBitsize = bits::NumUsedBits(onesRangeLen - 1);
    numBytesRangesEncVint += (onesRangeLenBitsize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    numBitsRangesEncArith = onesRangeLenBitsize > 0 ? onesRangeLenBitsize - 1 : 0;
    ++ranges1SizesFreqs[onesRangeLenBitsize];
    onesRangeLen = 0;
  }
  // Compute arithmetic encoding size.
  u64 diffsSizesTotalFreq = 0, ranges0_sizes_total_freq = 0, ranges1SizesTotalFreq = 0;
  for (u32 i = 0; i < diffsSizesFreqs.size(); ++i) diffsSizesTotalFreq += diffsSizesFreqs[i];
  for (u32 i = 0; i < ranges0SizesFreqs.size(); ++i) ranges0_sizes_total_freq += ranges0SizesFreqs[i];
  for (u32 i = 0; i < ranges1SizesFreqs.size(); ++i) ranges1SizesTotalFreq += ranges1SizesFreqs[i];
  // Compute number of bits for arith encoded diffs sizes.
  double numSizesBitsDiffsEncArith = 0;
  u32 nonzeroDiffsSizesFreqsEnd = 0;
  for (u32 i = 0; i < diffsSizesFreqs.size(); ++i)
  {
    if (diffsSizesFreqs[i] > 0)
    {
      double prob = double(diffsSizesFreqs[i]) / diffsSizesTotalFreq;
      numSizesBitsDiffsEncArith += - prob * log(prob) / log(2);
      nonzeroDiffsSizesFreqsEnd = i + 1;
    }
  }
  vector<u8> diffsSizesFreqsSerial;
  for (u32 i = 0; i < nonzeroDiffsSizesFreqsEnd; ++i) VarintEncode(diffsSizesFreqsSerial, diffsSizesFreqs[i]);
  u64 numBytesDiffsEncArith = 4 + diffsSizesFreqsSerial.size() + (u64(numSizesBitsDiffsEncArith * diffsSizesTotalFreq + 0.999) + 7) / 8 + (numBitsDiffsEncArith + 7) /8;
  // Compute number of bits for arith encoded ranges sizes.
  double numSizesBitsRanges0EncArith = 0;
  u32 nonzeroRanges0SizesFreqsEnd = 0;
  for (u32 i = 0; i < ranges0SizesFreqs.size(); ++i)
  {
    if (ranges0SizesFreqs[i] > 0)
    {
      double prob = double(ranges0SizesFreqs[i]) / ranges0_sizes_total_freq;
      numSizesBitsRanges0EncArith += - prob * log(prob) / log(2);
      nonzeroRanges0SizesFreqsEnd = i + 1;
    }
  }
  double numSizesBitsRanges1EncArith = 0;
  u32 nonzeroRanges1SizesFreqsEnd = 0;
  for (u32 i = 0; i < ranges1SizesFreqs.size(); ++i)
  {
    if (ranges1SizesFreqs[i] > 0)
    {
      double prob = double(ranges1SizesFreqs[i]) / ranges1SizesTotalFreq;
      numSizesBitsRanges1EncArith += - prob * log(prob) / log(2);
      nonzeroRanges1SizesFreqsEnd = i + 1;
    }
  }
  vector<u8> ranges0SizesFreqsSerial, ranges1SizesFreqsSerial;
  for (u32 i = 0; i < nonzeroRanges0SizesFreqsEnd; ++i) VarintEncode(ranges0SizesFreqsSerial, ranges0SizesFreqs[i]);
  for (u32 i = 0; i < nonzeroRanges1SizesFreqsEnd; ++i) VarintEncode(ranges1SizesFreqsSerial, ranges1SizesFreqs[i]);
  u64 numBytesRangesEncArith = 4 + ranges0SizesFreqsSerial.size() + ranges1SizesFreqsSerial.size() +
    (u64(numSizesBitsRanges0EncArith * ranges0_sizes_total_freq + 0.999) + 7) / 8 + (u64(numSizesBitsRanges1EncArith * ranges1SizesTotalFreq + 0.999) + 7) / 8 +
    (numBitsRangesEncArith + 7) / 8;

  // Find minimum among 4 types of encoding.
  vector<u64> numBytesPerEnc = {numBytesDiffsEncVint, numBytesRangesEncVint, numBytesDiffsEncArith, numBytesRangesEncArith};
  u32 encType = 0;
  if (chosenEncType != -1) { CHECK(0 <= chosenEncType && chosenEncType <= 3, ()); encType = chosenEncType; }
  else if (numBytesPerEnc[0] <= numBytesPerEnc[1] && numBytesPerEnc[0] <= numBytesPerEnc[2] && numBytesPerEnc[0] <= numBytesPerEnc[3]) encType = 0;
  else if (numBytesPerEnc[1] <= numBytesPerEnc[0] && numBytesPerEnc[1] <= numBytesPerEnc[2] && numBytesPerEnc[1] <= numBytesPerEnc[3]) encType = 1;
  else if (numBytesPerEnc[2] <= numBytesPerEnc[0] && numBytesPerEnc[2] <= numBytesPerEnc[1] && numBytesPerEnc[2] <= numBytesPerEnc[3]) encType = 2;
  else if (numBytesPerEnc[3] <= numBytesPerEnc[0] && numBytesPerEnc[3] <= numBytesPerEnc[1] && numBytesPerEnc[3] <= numBytesPerEnc[2]) encType = 3;

  if (encType == 0)
  {
    // Diffs-Varint encoding.

    int64_t prevOnePos = -1;
    bool is_empty = posOnes.empty();
    // Encode encoding type and first diff.
    if (is_empty)
    {
      VarintEncode(writer, encType + (1 << 2));
    }
    else
    {
      VarintEncode(writer, encType + (0 << 2) + ((posOnes[0] - prevOnePos - 1) << 3));
      prevOnePos = posOnes[0];
    }
    for (u32 i = 1; i < posOnes.size(); ++i)
    {
      CHECK_GREATER(posOnes[i], prevOnePos, ());
      // Encode one's pos (diff - 1).
      VarintEncode(writer, posOnes[i] - prevOnePos - 1);
      prevOnePos = posOnes[i];
    }
  }
  else if (encType == 2)
  {
    // Diffs-Arith encoding.
    
    // Encode encoding type plus number of freqs in the table.
    VarintEncode(writer, encType + (nonzeroDiffsSizesFreqsEnd << 2));
    // Encode freqs table.
    writer.Write(diffsSizesFreqsSerial.data(), diffsSizesFreqsSerial.size());
    u64 tmpOffset = 0;
    MemReader diffsSizesFreqsSerialReader(diffsSizesFreqsSerial.data(), diffsSizesFreqsSerial.size());
    vector<u32> distrTable = SerialFreqsToDistrTable(
      diffsSizesFreqsSerialReader, tmpOffset, nonzeroDiffsSizesFreqsEnd
    );

    {
      // First stage. Encode all bits sizes of all diffs using ArithmeticEncoder.
      ArithmeticEncoder arithEnc(distrTable);
      int64_t prevOnePos = -1;
      u64 cntElements = 0;
      for (u64 i = 0; i < posOnes.size(); ++i)
      {
        CHECK_GREATER(posOnes[i], prevOnePos, ());
        u32 bitsUsed = bits::NumUsedBits(posOnes[i] - prevOnePos - 1);
        arithEnc.Encode(bitsUsed);
        ++cntElements;
        prevOnePos = posOnes[i];
      }
      vector<u8> serialSizesEnc = arithEnc.Finalize();
      // Store number of compressed elements.
      VarintEncode(writer, cntElements);
      // Store compressed size of encoded sizes.
      VarintEncode(writer, serialSizesEnc.size());
      // Store serial sizes.
      writer.Write(serialSizesEnc.data(), serialSizesEnc.size());
    }
    {
      // Second Stage. Encode all bits of all diffs using BitWriter.
      BitWriter bitWriter(writer);
      int64_t prevOnePos = -1;
      u64 totalReadBits = 0;
      u64 totalReadCnts = 0;
      for (u64 i = 0; i < posOnes.size(); ++i)
      {
        CHECK_GREATER(posOnes[i], prevOnePos, ());
        // Encode one's pos (diff - 1).
        u64 diff = posOnes[i] - prevOnePos - 1;
        u32 bitsUsed = bits::NumUsedBits(diff);
        if (bitsUsed > 1)
        {
          // Most significant bit is always 1 for non-zero diffs, so don't store it.
          --bitsUsed;
          bitWriter.Write(diff, bitsUsed);
          totalReadBits += bitsUsed;
          ++totalReadCnts;
        }
        prevOnePos = posOnes[i];
      }
    }
  }
  else if (encType == 1)
  {
    // Ranges-Varint encoding.
    
    // If bit vector starts with 1.
    bool isFirstOne = posOnes.size() > 0 && posOnes.front() == 0;
    // Encode encoding type plus flag if first is 1.
    VarintEncode(writer, encType + ((isFirstOne ? 1 : 0) << 2));
    int64_t prevOnePos = -1;
    u64 onesRangeLen = 0;
    for (u32 i = 0; i < posOnes.size(); ++i)
    {
      CHECK_GREATER(posOnes[i], prevOnePos, ());
      if (posOnes[i] - prevOnePos > 1)
      {
        if (onesRangeLen > 0)
        {
          // Encode ones range size - 1.
          VarintEncode(writer, onesRangeLen - 1);
          onesRangeLen = 0;
        }
        // Encode zeros range size - 1.
        VarintEncode(writer, posOnes[i] - prevOnePos - 2);
      }
      ++onesRangeLen;
      prevOnePos = posOnes[i];
    }
    if (onesRangeLen > 0)
    {
      // Encode last ones range size.
      VarintEncode(writer, onesRangeLen - 1);
      onesRangeLen = 0;
    }
  }
  else if (encType == 3)
  {
    // Ranges-Arith encoding.

    // If bit vector starts with 1.
    bool isFirstOne = posOnes.size() > 0 && posOnes.front() == 0;
    // Encode encoding type plus flag if first is 1 plus count of sizes freqs.
    VarintEncode(writer, encType + ((isFirstOne ? 1 : 0) << 2) + (nonzeroRanges0SizesFreqsEnd << 3));
    VarintEncode(writer, nonzeroRanges1SizesFreqsEnd);
    // Encode freqs table.
    writer.Write(ranges0SizesFreqsSerial.data(), ranges0SizesFreqsSerial.size());
    writer.Write(ranges1SizesFreqsSerial.data(), ranges1SizesFreqsSerial.size());
    // Create distr tables.
    u64 tmpOffset = 0;
    MemReader ranges0SizesFreqsSerialReader(ranges0SizesFreqsSerial.data(), ranges0SizesFreqsSerial.size());
    vector<u32> distrTable0 = SerialFreqsToDistrTable(
      ranges0SizesFreqsSerialReader, tmpOffset, nonzeroRanges0SizesFreqsEnd
    );
    tmpOffset = 0;
    MemReader ranges1SizesFreqsSerialReader(ranges1SizesFreqsSerial.data(), ranges1SizesFreqsSerial.size());
    vector<u32> distrTable1 = SerialFreqsToDistrTable(
      ranges1SizesFreqsSerialReader, tmpOffset, nonzeroRanges1SizesFreqsEnd
    );

    {
      // First stage, encode all ranges bits sizes using ArithmeticEncoder.

      // Encode number of compressed elements.
      ArithmeticEncoder arith_enc0(distrTable0), arith_enc1(distrTable1);
      int64_t prevOnePos = -1;
      u64 onesRangeLen = 0;
      // Total number of compressed elements (ranges sizes).
      u64 cntElements0 = 0, cntElements1 = 0;
      for (u32 i = 0; i < posOnes.size(); ++i)
      {
        CHECK_GREATER(posOnes[i], prevOnePos, ());
        if (posOnes[i] - prevOnePos > 1)
        {
          if (onesRangeLen > 0)
          {
            // Encode ones range bits size.
            u32 bitsUsed = bits::NumUsedBits(onesRangeLen - 1);
            arith_enc1.Encode(bitsUsed);
            ++cntElements1;
            onesRangeLen = 0;
          }
          // Encode zeros range bits size - 1.
          u32 bitsUsed = bits::NumUsedBits(posOnes[i] - prevOnePos - 2);
          arith_enc0.Encode(bitsUsed);
          ++cntElements0;
        }
        ++onesRangeLen;
        prevOnePos = posOnes[i];
      }
      if (onesRangeLen > 0)
      {
        // Encode last ones range size - 1.
        u32 bitsUsed = bits::NumUsedBits(onesRangeLen - 1);
        arith_enc1.Encode(bitsUsed);
        ++cntElements1;
        onesRangeLen = 0;
      }
      vector<u8> serial0SizesEnc = arith_enc0.Finalize(), serial1SizesEnc = arith_enc1.Finalize();
      // Store number of compressed elements.
      VarintEncode(writer, cntElements0);
      VarintEncode(writer, cntElements1);
      // Store size of encoded bits sizes.
      VarintEncode(writer, serial0SizesEnc.size());
      VarintEncode(writer, serial1SizesEnc.size());
      // Store serial sizes.
      writer.Write(serial0SizesEnc.data(), serial0SizesEnc.size());
      writer.Write(serial1SizesEnc.data(), serial1SizesEnc.size());
    }

    {
      // Second stage, encode all ranges bits using BitWriter.
      BitWriter bitWriter(writer);
      int64_t prevOnePos = -1;
      u64 onesRangeLen = 0;
      for (u32 i = 0; i < posOnes.size(); ++i)
      {
        CHECK_GREATER(posOnes[i], prevOnePos, ());
        if (posOnes[i] - prevOnePos > 1)
        {
          if (onesRangeLen > 0)
          {
            // Encode ones range bits size.
            u32 bitsUsed = bits::NumUsedBits(onesRangeLen - 1);
            if (bitsUsed > 1)
            {
              // Most significant bit for non-zero values is always 1, don't encode it.
              --bitsUsed;
              bitWriter.Write(onesRangeLen - 1, bitsUsed);
            }
            onesRangeLen = 0;
          }
          // Encode zeros range bits size - 1.
          u32 bitsUsed = bits::NumUsedBits(posOnes[i] - prevOnePos - 2);
          if (bitsUsed > 1)
          {
            // Most significant bit for non-zero values is always 1, don't encode it.
            --bitsUsed;
            bitWriter.Write(posOnes[i] - prevOnePos - 2, bitsUsed);
          }
        }
        ++onesRangeLen;
        prevOnePos = posOnes[i];
      }
      if (onesRangeLen > 0)
      {
        // Encode last ones range size - 1.
        u32 bitsUsed = bits::NumUsedBits(onesRangeLen - 1);
        if (bitsUsed > 1)
        {
          // Most significant bit for non-zero values is always 1, don't encode it.
          --bitsUsed;
          bitWriter.Write(onesRangeLen - 1, bitsUsed);
        }
        onesRangeLen = 0;
      }
    }
  }
}

vector<u32> DecodeCompressedBitVector(Reader & reader) {
  u64 serialSize = reader.Size();
  vector<u32> posOnes;
  u64 decodeOffset = 0;
  u64 header = VarintDecode(reader, decodeOffset);
  u32 encType = header & 3;
  CHECK_LESS(encType, 4, ());
  if (encType == 0)
  {
    // Diffs-Varint encoded.
    int64_t prevOnePos = -1;
    // For non-empty vectors first diff is taken from header number.
    bool is_empty = (header & 4) != 0;
    if (!is_empty)
    {
      posOnes.push_back(header >> 3);
      prevOnePos = posOnes.back();
    }
    while (decodeOffset < serialSize)
    {
      posOnes.push_back(prevOnePos + VarintDecode(reader, decodeOffset) + 1);
      prevOnePos = posOnes.back();
    }
  }
  else if (encType == 2)
  {
    // Diffs-Arith encoded.
    u64 freqsCnt = header >> 2;
    vector<u32> distrTable = SerialFreqsToDistrTable(reader, decodeOffset, freqsCnt);
    u64 cntElements = VarintDecode(reader, decodeOffset);
    u64 encSizesBytesize = VarintDecode(reader, decodeOffset);
    vector<u32> bitsUsedVec;
    Reader * arithDecReader = reader.CreateSubReader(decodeOffset, encSizesBytesize);
    ArithmeticDecoder arithDec(*arithDecReader, distrTable);
    for (u64 i = 0; i < cntElements; ++i) bitsUsedVec.push_back(arithDec.Decode());
    decodeOffset += encSizesBytesize;
    Reader * bitReaderReader = reader.CreateSubReader(decodeOffset, serialSize - decodeOffset);
    BitReader bitReader(*bitReaderReader);
    int64_t prevOnePos = -1;
    for (u64 i = 0; i < cntElements; ++i)
    {
      u32 bitsUsed = bitsUsedVec[i];
      u64 diff = 0;
      if (bitsUsed > 0) diff = ((u64(1) << (bitsUsed - 1)) | bitReader.Read(bitsUsed - 1)) + 1; else diff = 1;
      posOnes.push_back(prevOnePos + diff);
      prevOnePos += diff;
    }
    decodeOffset = serialSize;
  }
  else if (encType == 1)
  {
    // Ranges-Varint encoding.
    
    // If bit vector starts with 1.
    bool isFirstOne = ((header >> 2) & 1) == 1;
    u64 sum = 0;
    while (decodeOffset < serialSize)
    {
      u64 zerosRangeSize = 0;
      // Don't read zero range size for the first time if first bit is 1.
      if (!isFirstOne) zerosRangeSize = VarintDecode(reader, decodeOffset) + 1; else isFirstOne = false;
      u64 onesRangeSize = VarintDecode(reader, decodeOffset) + 1;
      sum += zerosRangeSize;
      for (u64 i = sum; i < sum + onesRangeSize; ++i) posOnes.push_back(i);
      sum += onesRangeSize;
    }
  }
  else if (encType == 3)
  {
    // Ranges-Arith encoding.

    // If bit vector starts with 1.
    bool isFirstOne = ((header >> 2) & 1) == 1;
    u64 freqs0Cnt = header >> 3, freqs1Cnt = VarintDecode(reader, decodeOffset);
    vector<u32> distrTable0 = SerialFreqsToDistrTable(reader, decodeOffset, freqs0Cnt);
    vector<u32> distrTable1 = SerialFreqsToDistrTable(reader, decodeOffset, freqs1Cnt);
    u64 cntElements0 = VarintDecode(reader, decodeOffset), cntElements1 = VarintDecode(reader, decodeOffset);
    u64 enc0SizesBytesize = VarintDecode(reader, decodeOffset), enc1SizesBytesize = VarintDecode(reader, decodeOffset);
    Reader * arithDec0Reader = reader.CreateSubReader(decodeOffset, enc0SizesBytesize);
    ArithmeticDecoder arithDec0(*arithDec0Reader, distrTable0);
    vector<u32> bitsSizes0;
    for (u64 i = 0; i < cntElements0; ++i) bitsSizes0.push_back(arithDec0.Decode());
    decodeOffset += enc0SizesBytesize;
    Reader * arithDec1Reader = reader.CreateSubReader(decodeOffset, enc1SizesBytesize);
    ArithmeticDecoder arith_dec1(*arithDec1Reader, distrTable1);
    vector<u32> bitsSizes1;
    for (u64 i = 0; i < cntElements1; ++i) bitsSizes1.push_back(arith_dec1.Decode());
    decodeOffset += enc1SizesBytesize;
    Reader * bitReaderReader = reader.CreateSubReader(decodeOffset, serialSize - decodeOffset);
    BitReader bitReader(*bitReaderReader);
    u64 sum = 0, i0 = 0, i1 = 0;
    while (i0 < cntElements0 && i1 < cntElements1)
    {
      u64 zerosRangeSize = 0;
      // Don't read zero range size for the first time if first bit is 1.
      if (!isFirstOne)
      {
        u32 bitsUsed = bitsSizes0[i0];
        if (bitsUsed > 0) zerosRangeSize = ((u64(1) << (bitsUsed - 1)) | bitReader.Read(bitsUsed - 1)) + 1; else zerosRangeSize = 1;
        ++i0;
      }
      else isFirstOne = false;
      u64 onesRangeSize = 0;
      u32 bitsUsed = bitsSizes1[i1];
      if (bitsUsed > 0) onesRangeSize = ((u64(1) << (bitsUsed - 1)) | bitReader.Read(bitsUsed - 1)) + 1; else onesRangeSize = 1;
      ++i1;
      sum += zerosRangeSize;
      for (u64 j = sum; j < sum + onesRangeSize; ++j) posOnes.push_back(j);
      sum += onesRangeSize;
    }
    CHECK(i0 == cntElements0 && i1 == cntElements1, ());
    decodeOffset = serialSize;
  }
  return posOnes;
}
