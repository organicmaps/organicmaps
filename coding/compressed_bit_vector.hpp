// Author: Artyom.
// Module for compressing/decompressing bit vectors.
// Usage:
//   vector<uint8_t> comprBits1;
//   MemWriter< vector<uint8_t> > writer(comprBits1);
//   // Create a bit vector by storing increasing positions of ones.
//   vector<uint32_t> posOnes1 = {12, 34, 75}, posOnes2 = {10, 34, 95};
//   // Compress some vectors.
//   BuildCompressedBitVector(writer, posOnes1);
//   MemReader reader(comprBits1.data(), comprBits1.size());
//   // Decompress compressed vectors before operations.
//   MemReader reader(comprBits1.data(), comprBits1.size());
//   posOnes1 = DecodeCompressedBitVector(reader);
//   // Intersect two vectors.
//   vector<uint32_t> andRes = BitVectorsAnd(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());
//   // Unite two vectors.
//   vector<uint32_t> orRes = BitVectorsAnd(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());
//   // Sub-and two vectors (second vector-set is a subset of first vector-set as bit vectors,
//   // so that second vector size should be equal to number of ones of the first vector).
//   vector<uint32_t> subandRes = BitVectorsSubAnd(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());

#pragma once

#include "base/assert.hpp"
#include "std/iterator.hpp"
#include "std/cstdint.hpp"
#include "std/vector.hpp"

// Forward declare used Reader/Writer.
class Reader;
class Writer;

// Build compressed bit vector from vector of ones bits positions, you may provide chosenEncType - encoding
// type of the result, otherwise encoding type is chosen to achieve maximum compression.
// Encoding types are: 0 - Diffs/Varint, 1 - Ranges/Varint, 2 - Diffs/Arith, 3 - Ranges/Arith.
// ("Diffs" creates a compressed array of pos diffs between ones inside source bit vector,
//  "Ranges" creates a compressed array of lengths of zeros and ones ranges,
//  "Varint" encodes resulting sizes using varint encoding,
//  "Arith" encodes resulting sizes using arithmetic encoding).
void BuildCompressedBitVector(Writer & writer, vector<uint32_t> const & posOnes, int chosenEncType = -1);
// Decodes compressed bit vector to uncompressed array of ones positions.
vector<uint32_t> DecodeCompressedBitVector(Reader & reader);

// Intersects two bit vectors based on theirs begin and end iterators.
// Returns resulting positions of ones.
template <typename It1T, typename It2T>
vector<uint32_t> BitVectorsAnd(It1T begin1, It1T end1, It2T begin2, It2T end2)
{
  vector<uint32_t> result;

  It1T it1 = begin1;
  It2T it2 = begin2;
  while (it1 != end1 && it2 != end2)
  {
    uint32_t pos1 = *it1, pos2 = *it2;
    if (pos1 == pos2)
    {
      result.push_back(pos1);
      ++it1;
      ++it2;
    }
    else if (pos1 < pos2) { ++it1; }
    else if (pos1 > pos2) { ++it2; }
  }
  return result;
}

// Unites two bit vectors based on theirs begin and end iterators.
// Returns resulting positions of ones.
template <typename It1T, typename It2T>
vector<uint32_t> BitVectorsOr(It1T begin1, It1T end1, It2T begin2, It2T end2)
{
  vector<uint32_t> result;

  It1T it1 = begin1;
  It2T it2 = begin2;
  while (it1 != end1 && it2 != end2)
  {
    uint32_t pos1 = *it1, pos2 = *it2;
    if (pos1 == pos2)
    {
      result.push_back(pos1);
      ++it1;
      ++it2;
    }
    else if (pos1 < pos2)
    {
      result.push_back(pos1);
      ++it1;      
    }
    else  // pos1 > pos2
    {
      result.push_back(pos2);
      ++it2;
    }
  }

  for (; it1 != end1; ++it1)
    result.push_back(*it1);
  for (; it2 != end2; ++it2)
    result.push_back(*it2);
  return result;
}

// Intersects first vector with second vector, when second vector is a subset of the first vector,
// second bit vector should have size equal to first vector's number of ones.
// Returns resulting positions of ones.
template <typename It1T, typename It2T>
vector<uint32_t> BitVectorsSubAnd(It1T begin1, It1T end1, It2T begin2, It2T end2)
{
  vector<uint32_t> result;

  It1T it1 = begin1;
  It2T it2 = begin2;
  uint64_t index = 0;
  for (; it1 != end1 && it2 != end2; ++it2) {
    advance(it1, *it2 - index);
    index = *it2;
    result.push_back(*it1);
  }
  CHECK((it2 == end2), ());
  return result;
}
