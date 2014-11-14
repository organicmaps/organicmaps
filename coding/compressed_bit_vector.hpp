// Author: Artyom.
// Module for compressing/decompressing bit vectors.
// Usage:
//   vector<uint8_t> compr_bits1;
//   MemWriter< vector<uint8_t> > writer(compr_bits1);
//   // Create a bit vector by storing increasing positions of ones.
//   vector<uint32_t> pos_ones1 = {12, 34, 75}, pos_ones2 = {10, 34, 95};
//   // Compress some vectors.
//   BuildCompressedBitVector(writer, pos_ones1);
//   MemReader reader(compr_bits1.data(), compr_bits1.size());
//   // Decompress compressed vectors before operations.
//   MemReader reader(compr_bits1.data(), compr_bits1.size());
//   pos_ones1 = DecodeCompressedBitVector(reader);
//   // Intersect two vectors.
//   vector<uint32_t> and_res = BitVectorsAnd(pos_ones1.begin(), pos_ones1.end(), pos_ones2.begin(), pos_ones2.end());
//   // Unite two vectors.
//   vector<uint32_t> or_res = BitVectorsAnd(pos_ones1.begin(), pos_ones1.end(), pos_ones2.begin(), pos_ones2.end());
//   // Sub-and two vectors (second vector-set is a subset of first vector-set as bit vectors,
//   // so that second vector size should be equal to number of ones of the first vector).
//   vector<uint32_t> suband_res = BitVectorsSubAnd(pos_ones1.begin(), pos_ones1.end(), pos_ones2.begin(), pos_ones2.end());

#pragma once

#include "../base/assert.hpp"
#include "../std/stdint.hpp"
#include "../std/vector.hpp"

// Forward declare used Reader/Writer.
class Reader;
class Writer;

// Build compressed bit vector from vector of ones bits positions, you may provide chosen_enc_type - encoding
// type of the result, otherwise encoding type is chosen to achieve maximum compression.
// Encoding types are: 0 - Diffs/Varint, 1 - Ranges/Varint, 2 - Diffs/Arith, 3 - Ranges/Arith.
// ("Diffs" creates a compressed array of pos diffs between ones inside source bit vector,
//  "Ranges" creates a compressed array of lengths of zeros and ones ranges,
//  "Varint" encodes resulting sizes using varint encoding,
//  "Arith" encodes resulting sizes using arithmetic encoding).
void BuildCompressedBitVector(Writer & writer, std::vector<uint32_t> const & pos_ones, int chosen_enc_type = -1);
// Decodes compressed bit vector to uncompressed array of ones positions.
std::vector<uint32_t> DecodeCompressedBitVector(Reader & reader);

// Intersects two bit vectors based on theirs begin and end iterators.
// Returns resulting positions of ones.
template <typename It1T, typename It2T>
std::vector<uint32_t> BitVectorsAnd(It1T begin1, It1T end1, It2T begin2, It2T end2) {
  std::vector<uint32_t> result;

  It1T it1 = begin1;
  It2T it2 = begin2;
  while (it1 != end1 && it2 != end2) {
    uint32_t pos1 = *it1, pos2 = *it2;
    if (pos1 == pos2) {
      result.push_back(pos1);
      ++it1;
      ++it2;
    } else if (pos1 < pos2) { ++it1; }
    else if (pos1 > pos2) { ++it2; }
  }
  return result;
}

// Unites two bit vectors based on theirs begin and end iterators.
// Returns resulting positions of ones.
template <typename It1T, typename It2T>
std::vector<uint32_t> BitVectorsOr(It1T begin1, It1T end1, It2T begin2, It2T end2) {
  std::vector<uint32_t> result;

  It1T it1 = begin1;
  It2T it2 = begin2;
  while (it1 != end1 && it2 != end2) {
    uint32_t pos1 = *it1, pos2 = *it2;
    if (pos1 == pos2) {
      result.push_back(pos1);
      ++it1;
      ++it2;
    } else if (pos1 < pos2) {
      result.push_back(pos1);
      ++it1;      
    } else if (pos1 > pos2) {
      result.push_back(pos2);
      ++it2;
    }
  }
  if (it2 == end2) {
    while (it1 != end1) {
      uint32_t pos1 = *it1;
      result.push_back(pos1);
      ++it1;
    }
  } else {
    while (it2 != end2) {
      uint32_t pos2 = *it2;
      result.push_back(pos2);
      ++it2;
    }
  }
  return result;
}

// Intersects first vector with second vector, when second vector is a subset of the first vector,
// second bit vector should have size equal to first vector's number of ones.
// Returns resulting positions of ones.
template <typename It1T, typename It2T>
std::vector<uint32_t> BitVectorsSubAnd(It1T begin1, It1T end1, It2T begin2, It2T end2) {
  std::vector<uint32_t> result;

  It1T it1 = begin1;
  It2T it2 = begin2;
  uint64_t index2 = 0;
  for (; it1 != end1 && it2 != end2; ++it1, ++index2) {
    uint32_t pos1 = *it1, pos2 = *it2;
    if (pos2 == index2) {
      result.push_back(pos1);
      ++it2;
    }
  }
  ASSERT((it2 == end2), ());
  return result;
}
