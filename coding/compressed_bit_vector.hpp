// Author: Artyom.
// Module for compressing/decompressing bit vectors.
// Usage:
//   vector<u8> comprBits1;
//   MemWriter< vector<u8> > writer(comprBits1);
//   // Create a bit vector by storing increasing positions of ones.
//   vector<u32> posOnes1 = {12, 34, 75}, posOnes2 = {10, 34, 95};
//   // Compress some vectors.
//   BuildCompressedBitVector(writer, posOnes1);
//   MemReader reader(comprBits1.data(), comprBits1.size());
//   // Decompress compressed vectors before operations.
//   MemReader reader(comprBits1.data(), comprBits1.size());
//   posOnes1 = DecodeCompressedBitVector(reader);
//   // Intersect two vectors.
//   vector<u32> andRes = BitVectorsAnd(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());
//   // Unite two vectors.
//   vector<u32> orRes = BitVectorsAnd(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());
//   // Sub-and two vectors (second vector-set is a subset of first vector-set as bit vectors,
//   // so that second vector size should be equal to number of ones of the first vector).
//   vector<u32> subandRes = BitVectorsSubAnd(posOnes1.begin(), posOnes1.end(), posOnes2.begin(), posOnes2.end());

#pragma once

#include "../base/assert.hpp"
#include "../std/stdint.hpp"
#include "../std/vector.hpp"

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

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
void BuildCompressedBitVector(Writer & writer, vector<u32> const & posOnes, int chosenEncType = -1);
// Decodes compressed bit vector to uncompressed array of ones positions.
vector<u32> DecodeCompressedBitVector(Reader & reader);

// Intersects two bit vectors based on theirs begin and end iterators.
// Returns resulting positions of ones.
template <typename It1T, typename It2T>
vector<u32> BitVectorsAnd(It1T begin1, It1T end1, It2T begin2, It2T end2)
{
  vector<u32> result;

  It1T it1 = begin1;
  It2T it2 = begin2;
  while (it1 != end1 && it2 != end2)
  {
    u32 pos1 = *it1, pos2 = *it2;
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
vector<u32> BitVectorsOr(It1T begin1, It1T end1, It2T begin2, It2T end2)
{
  vector<u32> result;

  It1T it1 = begin1;
  It2T it2 = begin2;
  while (it1 != end1 && it2 != end2)
  {
    u32 pos1 = *it1, pos2 = *it2;
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
    else if (pos1 > pos2)
    {
      result.push_back(pos2);
      ++it2;
    }
  }
  if (it2 == end2)
  {
    while (it1 != end1)
    {
      u32 pos1 = *it1;
      result.push_back(pos1);
      ++it1;
    }
  }
  else
  {
    while (it2 != end2)
    {
      u32 pos2 = *it2;
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
vector<u32> BitVectorsSubAnd(It1T begin1, It1T end1, It2T begin2, It2T end2)
{
  vector<u32> result;

  It1T it1 = begin1;
  It2T it2 = begin2;
  u64 index2 = 0;
  for (; it1 != end1 && it2 != end2; ++it1, ++index2)
  {
    u64 pos1 = *it1, pos2 = *it2;
    if (pos2 == index2)
    {
      result.push_back(pos1);
      ++it2;
    }
  }
  CHECK((it2 == end2), ());
  return result;
}
