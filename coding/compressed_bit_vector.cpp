#include "coding/compressed_bit_vector.hpp"

#include "coding/arithmetic_codec.hpp"
#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"
#include "coding/varint_misc.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"

#include "std/cmath.hpp"
#include "std/unique_ptr.hpp"

namespace {
  vector<uint32_t> SerialFreqsToDistrTable(Reader & reader, uint64_t & decodeOffset, uint64_t cnt)
  {
    vector<uint32_t> freqs;
    for (uint64_t i = 0; i < cnt; ++i) freqs.push_back(VarintDecode(reader, decodeOffset));
    return FreqsToDistrTable(freqs);
  }
}

void BuildCompressedBitVector(Writer & writer, vector<uint32_t> const & posOnes, int chosenEncType)
{
  uint32_t const BLOCK_SIZE = 7;
  // First stage of compression is analysis run through data ones.
  uint64_t numBytesDiffsEncVint = 0, numBytesRangesEncVint = 0, numBitsDiffsEncArith = 0, numBitsRangesEncArith = 0;
  int64_t prevOnePos = -1;
  uint64_t onesRangeLen = 0;
  vector<uint32_t> diffsSizesFreqs(65, 0), ranges0SizesFreqs(65, 0), ranges1SizesFreqs(65, 0);
  for (uint32_t i = 0; i < posOnes.size(); ++i)
  {
    CHECK_LESS(prevOnePos, posOnes[i], ());
    // Accumulate size of diff encoding.
    uint64_t diff = posOnes[i] - prevOnePos;
    uint32_t diffBitsize = bits::NumUsedBits(diff - 1);
    numBytesDiffsEncVint += (diffBitsize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    numBitsDiffsEncArith += diffBitsize > 0 ? diffBitsize - 1 : 0;
    ++diffsSizesFreqs[diffBitsize];
    // Accumulate sizes of ranges encoding.
    if (posOnes[i] - prevOnePos > 1)
    {
      if (onesRangeLen > 0)
      {
        // Accumulate size of ones-range encoding.
        uint32_t onesRangeLenBitsize = bits::NumUsedBits(onesRangeLen - 1);
        numBytesRangesEncVint += (onesRangeLenBitsize + BLOCK_SIZE - 1) / BLOCK_SIZE;
        numBitsRangesEncArith += onesRangeLenBitsize > 0 ? onesRangeLenBitsize - 1 : 0;
        ++ranges1SizesFreqs[onesRangeLenBitsize];
        onesRangeLen = 0;
      }
      // Accumulate size of zeros-range encoding.
      uint32_t zeros_range_len_bitsize = bits::NumUsedBits(posOnes[i] - prevOnePos - 2);
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
    uint32_t onesRangeLenBitsize = bits::NumUsedBits(onesRangeLen - 1);
    numBytesRangesEncVint += (onesRangeLenBitsize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    numBitsRangesEncArith = onesRangeLenBitsize > 0 ? onesRangeLenBitsize - 1 : 0;
    ++ranges1SizesFreqs[onesRangeLenBitsize];
    onesRangeLen = 0;
  }
  // Compute arithmetic encoding size.
  uint64_t diffsSizesTotalFreq = 0, ranges0_sizes_total_freq = 0, ranges1SizesTotalFreq = 0;
  for (uint32_t i = 0; i < diffsSizesFreqs.size(); ++i) diffsSizesTotalFreq += diffsSizesFreqs[i];
  for (uint32_t i = 0; i < ranges0SizesFreqs.size(); ++i) ranges0_sizes_total_freq += ranges0SizesFreqs[i];
  for (uint32_t i = 0; i < ranges1SizesFreqs.size(); ++i) ranges1SizesTotalFreq += ranges1SizesFreqs[i];
  // Compute number of bits for arith encoded diffs sizes.
  double numSizesBitsDiffsEncArith = 0;
  uint32_t nonzeroDiffsSizesFreqsEnd = 0;
  for (uint32_t i = 0; i < diffsSizesFreqs.size(); ++i)
  {
    if (diffsSizesFreqs[i] > 0)
    {
      double prob = double(diffsSizesFreqs[i]) / diffsSizesTotalFreq;
      numSizesBitsDiffsEncArith += - prob * log(prob) / log(2);
      nonzeroDiffsSizesFreqsEnd = i + 1;
    }
  }
  vector<uint8_t> diffsSizesFreqsSerial;
  for (uint32_t i = 0; i < nonzeroDiffsSizesFreqsEnd; ++i) VarintEncode(diffsSizesFreqsSerial, diffsSizesFreqs[i]);
  uint64_t numBytesDiffsEncArith = 4 + diffsSizesFreqsSerial.size() + (uint64_t(numSizesBitsDiffsEncArith * diffsSizesTotalFreq + 0.999) + 7) / 8 + (numBitsDiffsEncArith + 7) /8;
  // Compute number of bits for arith encoded ranges sizes.
  double numSizesBitsRanges0EncArith = 0;
  uint32_t nonzeroRanges0SizesFreqsEnd = 0;
  for (uint32_t i = 0; i < ranges0SizesFreqs.size(); ++i)
  {
    if (ranges0SizesFreqs[i] > 0)
    {
      double prob = double(ranges0SizesFreqs[i]) / ranges0_sizes_total_freq;
      numSizesBitsRanges0EncArith += - prob * log(prob) / log(2);
      nonzeroRanges0SizesFreqsEnd = i + 1;
    }
  }
  double numSizesBitsRanges1EncArith = 0;
  uint32_t nonzeroRanges1SizesFreqsEnd = 0;
  for (uint32_t i = 0; i < ranges1SizesFreqs.size(); ++i)
  {
    if (ranges1SizesFreqs[i] > 0)
    {
      double prob = double(ranges1SizesFreqs[i]) / ranges1SizesTotalFreq;
      numSizesBitsRanges1EncArith += - prob * log(prob) / log(2);
      nonzeroRanges1SizesFreqsEnd = i + 1;
    }
  }
  vector<uint8_t> ranges0SizesFreqsSerial, ranges1SizesFreqsSerial;
  for (uint32_t i = 0; i < nonzeroRanges0SizesFreqsEnd; ++i) VarintEncode(ranges0SizesFreqsSerial, ranges0SizesFreqs[i]);
  for (uint32_t i = 0; i < nonzeroRanges1SizesFreqsEnd; ++i) VarintEncode(ranges1SizesFreqsSerial, ranges1SizesFreqs[i]);
  uint64_t numBytesRangesEncArith = 4 + ranges0SizesFreqsSerial.size() + ranges1SizesFreqsSerial.size() +
    (uint64_t(numSizesBitsRanges0EncArith * ranges0_sizes_total_freq + 0.999) + 7) / 8 + (uint64_t(numSizesBitsRanges1EncArith * ranges1SizesTotalFreq + 0.999) + 7) / 8 +
    (numBitsRangesEncArith + 7) / 8;

  // Find minimum among 4 types of encoding.
  vector<uint64_t> numBytesPerEnc = {numBytesDiffsEncVint, numBytesRangesEncVint, numBytesDiffsEncArith, numBytesRangesEncArith};
  uint32_t encType = 0;
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
    for (uint32_t i = 1; i < posOnes.size(); ++i)
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
    uint64_t tmpOffset = 0;
    MemReader diffsSizesFreqsSerialReader(diffsSizesFreqsSerial.data(), diffsSizesFreqsSerial.size());
    vector<uint32_t> distrTable = SerialFreqsToDistrTable(
      diffsSizesFreqsSerialReader, tmpOffset, nonzeroDiffsSizesFreqsEnd
    );

    {
      // First stage. Encode all bits sizes of all diffs using ArithmeticEncoder.
      ArithmeticEncoder arithEnc(distrTable);
      int64_t prevOnePos = -1;
      uint64_t cntElements = 0;
      for (uint64_t i = 0; i < posOnes.size(); ++i)
      {
        CHECK_GREATER(posOnes[i], prevOnePos, ());
        uint32_t bitsUsed = bits::NumUsedBits(posOnes[i] - prevOnePos - 1);
        arithEnc.Encode(bitsUsed);
        ++cntElements;
        prevOnePos = posOnes[i];
      }
      vector<uint8_t> serialSizesEnc = arithEnc.Finalize();
      // Store number of compressed elements.
      VarintEncode(writer, cntElements);
      // Store compressed size of encoded sizes.
      VarintEncode(writer, serialSizesEnc.size());
      // Store serial sizes.
      writer.Write(serialSizesEnc.data(), serialSizesEnc.size());
    }
    {
      // Second Stage. Encode all bits of all diffs using BitWriter.
      BitWriter<Writer> bitWriter(writer);
      int64_t prevOnePos = -1;
      uint64_t totalReadBits = 0;
      uint64_t totalReadCnts = 0;
      for (uint64_t i = 0; i < posOnes.size(); ++i)
      {
        CHECK_GREATER(posOnes[i], prevOnePos, ());
        // Encode one's pos (diff - 1).
        uint64_t diff = posOnes[i] - prevOnePos - 1;
        uint32_t bitsUsed = bits::NumUsedBits(diff);
        if (bitsUsed > 1)
        {
          // Most significant bit is always 1 for non-zero diffs, so don't store it.
          --bitsUsed;
          for (size_t j = 0; j < bitsUsed; ++j)
            bitWriter.Write((diff >> j) & 1, 1);
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
    uint64_t onesRangeLen = 0;
    for (uint32_t i = 0; i < posOnes.size(); ++i)
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
    uint64_t tmpOffset = 0;
    MemReader ranges0SizesFreqsSerialReader(ranges0SizesFreqsSerial.data(), ranges0SizesFreqsSerial.size());
    vector<uint32_t> distrTable0 = SerialFreqsToDistrTable(
      ranges0SizesFreqsSerialReader, tmpOffset, nonzeroRanges0SizesFreqsEnd
    );
    tmpOffset = 0;
    MemReader ranges1SizesFreqsSerialReader(ranges1SizesFreqsSerial.data(), ranges1SizesFreqsSerial.size());
    vector<uint32_t> distrTable1 = SerialFreqsToDistrTable(
      ranges1SizesFreqsSerialReader, tmpOffset, nonzeroRanges1SizesFreqsEnd
    );

    {
      // First stage, encode all ranges bits sizes using ArithmeticEncoder.

      // Encode number of compressed elements.
      ArithmeticEncoder arith_enc0(distrTable0), arith_enc1(distrTable1);
      int64_t prevOnePos = -1;
      uint64_t onesRangeLen = 0;
      // Total number of compressed elements (ranges sizes).
      uint64_t cntElements0 = 0, cntElements1 = 0;
      for (uint32_t i = 0; i < posOnes.size(); ++i)
      {
        CHECK_GREATER(posOnes[i], prevOnePos, ());
        if (posOnes[i] - prevOnePos > 1)
        {
          if (onesRangeLen > 0)
          {
            // Encode ones range bits size.
            uint32_t bitsUsed = bits::NumUsedBits(onesRangeLen - 1);
            arith_enc1.Encode(bitsUsed);
            ++cntElements1;
            onesRangeLen = 0;
          }
          // Encode zeros range bits size - 1.
          uint32_t bitsUsed = bits::NumUsedBits(posOnes[i] - prevOnePos - 2);
          arith_enc0.Encode(bitsUsed);
          ++cntElements0;
        }
        ++onesRangeLen;
        prevOnePos = posOnes[i];
      }
      if (onesRangeLen > 0)
      {
        // Encode last ones range size - 1.
        uint32_t bitsUsed = bits::NumUsedBits(onesRangeLen - 1);
        arith_enc1.Encode(bitsUsed);
        ++cntElements1;
        onesRangeLen = 0;
      }
      vector<uint8_t> serial0SizesEnc = arith_enc0.Finalize(), serial1SizesEnc = arith_enc1.Finalize();
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
      BitWriter<Writer> bitWriter(writer);
      int64_t prevOnePos = -1;
      uint64_t onesRangeLen = 0;
      for (uint32_t i = 0; i < posOnes.size(); ++i)
      {
        CHECK_GREATER(posOnes[i], prevOnePos, ());
        if (posOnes[i] - prevOnePos > 1)
        {
          if (onesRangeLen > 0)
          {
            // Encode ones range bits size.
            uint32_t bitsUsed = bits::NumUsedBits(onesRangeLen - 1);
            if (bitsUsed > 1)
            {
              // Most significant bit for non-zero values is always 1, don't encode it.
              --bitsUsed;
              for (size_t j = 0; j < bitsUsed; ++j)
                bitWriter.Write(((onesRangeLen - 1) >> j) & 1, 1);
            }
            onesRangeLen = 0;
          }
          // Encode zeros range bits size - 1.
          uint32_t bitsUsed = bits::NumUsedBits(posOnes[i] - prevOnePos - 2);
          if (bitsUsed > 1)
          {
            // Most significant bit for non-zero values is always 1, don't encode it.
            --bitsUsed;
            for (size_t j = 0; j < bitsUsed; ++j)
              bitWriter.Write(((posOnes[i] - prevOnePos - 2) >> j) & 1, 1);
          }
        }
        ++onesRangeLen;
        prevOnePos = posOnes[i];
      }
      if (onesRangeLen > 0)
      {
        // Encode last ones range size - 1.
        uint32_t bitsUsed = bits::NumUsedBits(onesRangeLen - 1);
        if (bitsUsed > 1)
        {
          // Most significant bit for non-zero values is always 1, don't encode it.
          --bitsUsed;
          for (size_t j = 0; j < bitsUsed; ++j)
            bitWriter.Write(((onesRangeLen - 1) >> j) & 1, 1);
        }
        onesRangeLen = 0;
      }
    }
  }
}

vector<uint32_t> DecodeCompressedBitVector(Reader & reader) {
  uint64_t serialSize = reader.Size();
  vector<uint32_t> posOnes;
  uint64_t decodeOffset = 0;
  uint64_t header = VarintDecode(reader, decodeOffset);
  uint32_t encType = header & 3;
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
    uint64_t freqsCnt = header >> 2;
    vector<uint32_t> distrTable = SerialFreqsToDistrTable(reader, decodeOffset, freqsCnt);
    uint64_t cntElements = VarintDecode(reader, decodeOffset);
    uint64_t encSizesBytesize = VarintDecode(reader, decodeOffset);
    vector<uint32_t> bitsUsedVec;
    unique_ptr<Reader> arithDecReader(reader.CreateSubReader(decodeOffset, encSizesBytesize));
    ArithmeticDecoder arithDec(*arithDecReader, distrTable);
    for (uint64_t i = 0; i < cntElements; ++i) bitsUsedVec.push_back(arithDec.Decode());
    decodeOffset += encSizesBytesize;
    ReaderPtr<Reader> readerPtr(reader.CreateSubReader(decodeOffset, serialSize - decodeOffset));
    ReaderSource<ReaderPtr<Reader>> bitReaderSource(readerPtr);
    BitReader<ReaderSource<ReaderPtr<Reader>>> bitReader(bitReaderSource);
    int64_t prevOnePos = -1;
    for (uint64_t i = 0; i < cntElements; ++i)
    {
      uint32_t bitsUsed = bitsUsedVec[i];
      uint64_t diff = 0;
      if (bitsUsed > 0)
      {
        diff = static_cast<uint64_t>(1) << (bitsUsed - 1);
        for (size_t j = 0; j + 1 < bitsUsed; ++j)
          diff |= bitReader.Read(1) << j;
        ++diff;
      }
      else
      {
        diff = 1;
      }
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
    uint64_t sum = 0;
    while (decodeOffset < serialSize)
    {
      uint64_t zerosRangeSize = 0;
      // Don't read zero range size for the first time if first bit is 1.
      if (!isFirstOne) zerosRangeSize = VarintDecode(reader, decodeOffset) + 1; else isFirstOne = false;
      uint64_t onesRangeSize = VarintDecode(reader, decodeOffset) + 1;
      sum += zerosRangeSize;
      for (uint64_t i = sum; i < sum + onesRangeSize; ++i) posOnes.push_back(i);
      sum += onesRangeSize;
    }
  }
  else if (encType == 3)
  {
    // Ranges-Arith encoding.

    // If bit vector starts with 1.
    bool isFirstOne = ((header >> 2) & 1) == 1;
    uint64_t freqs0Cnt = header >> 3, freqs1Cnt = VarintDecode(reader, decodeOffset);
    vector<uint32_t> distrTable0 = SerialFreqsToDistrTable(reader, decodeOffset, freqs0Cnt);
    vector<uint32_t> distrTable1 = SerialFreqsToDistrTable(reader, decodeOffset, freqs1Cnt);
    uint64_t cntElements0 = VarintDecode(reader, decodeOffset), cntElements1 = VarintDecode(reader, decodeOffset);
    uint64_t enc0SizesBytesize = VarintDecode(reader, decodeOffset), enc1SizesBytesize = VarintDecode(reader, decodeOffset);
    unique_ptr<Reader> arithDec0Reader(reader.CreateSubReader(decodeOffset, enc0SizesBytesize));
    ArithmeticDecoder arithDec0(*arithDec0Reader, distrTable0);
    vector<uint32_t> bitsSizes0;
    for (uint64_t i = 0; i < cntElements0; ++i) bitsSizes0.push_back(arithDec0.Decode());
    decodeOffset += enc0SizesBytesize;
    unique_ptr<Reader> arithDec1Reader(reader.CreateSubReader(decodeOffset, enc1SizesBytesize));
    ArithmeticDecoder arith_dec1(*arithDec1Reader, distrTable1);
    vector<uint32_t> bitsSizes1;
    for (uint64_t i = 0; i < cntElements1; ++i) bitsSizes1.push_back(arith_dec1.Decode());
    decodeOffset += enc1SizesBytesize;
    ReaderPtr<Reader> readerPtr(reader.CreateSubReader(decodeOffset, serialSize - decodeOffset));
    ReaderSource<ReaderPtr<Reader>> bitReaderSource(readerPtr);
    BitReader<ReaderSource<ReaderPtr<Reader>>> bitReader(bitReaderSource);
    uint64_t sum = 0, i0 = 0, i1 = 0;
    while (i0 < cntElements0 && i1 < cntElements1)
    {
      uint64_t zerosRangeSize = 0;
      // Don't read zero range size for the first time if first bit is 1.
      if (!isFirstOne)
      {
        uint32_t bitsUsed = bitsSizes0[i0];
        if (bitsUsed > 0)
        {
          zerosRangeSize = static_cast<uint64_t>(1) << (bitsUsed - 1);
          for (size_t j = 0; j + 1 < bitsUsed; ++j)
            zerosRangeSize |= bitReader.Read(1) << j;
          ++zerosRangeSize;
        }
        else
        {
          zerosRangeSize = 1;
        }
        ++i0;
      }
      else isFirstOne = false;
      uint64_t onesRangeSize = 0;
      uint32_t bitsUsed = bitsSizes1[i1];
      if (bitsUsed > 0)
      {
        onesRangeSize = static_cast<uint64_t>(1) << (bitsUsed - 1);
        for (size_t j = 0; j + 1 < bitsUsed; ++j)
          onesRangeSize |= bitReader.Read(1) << j;
        ++onesRangeSize;
      }
      else
      {
        onesRangeSize = 1;
      }
      ++i1;
      sum += zerosRangeSize;
      for (uint64_t j = sum; j < sum + onesRangeSize; ++j) posOnes.push_back(j);
      sum += onesRangeSize;
    }
    CHECK(i0 == cntElements0 && i1 == cntElements1, ());
    decodeOffset = serialSize;
  }
  return posOnes;
}
