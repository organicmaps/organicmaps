// Author: Artyom Polkovnikov.
// Different variants of Varint encoding/decoding.

#pragma once

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "std/cstdint.hpp"
#include "std/vector.hpp"

// Encode Varint by appending to vector of bytes.
inline void VarintEncode(vector<uint8_t> & dst, uint64_t n)
{
  if (n == 0)
  {
    dst.push_back(0);
  }
  else
  {
    while (n != 0)
    {
      uint8_t b = n & 0x7F;
      n >>= 7;
      b |= n == 0 ? 0 : 0x80;
      dst.push_back(b);
    }
  }
}
// Encode varint using bytes Writer.
inline void VarintEncode(Writer & writer, uint64_t n)
{
  if (n == 0)
  {
    writer.Write(&n, 1);
  }
  else
  {
    while (n != 0)
    {
      uint8_t b = n & 0x7F;
      n >>= 7;
      b |= n == 0 ? 0 : 0x80;
      writer.Write(&b, 1);
    }
  }
}
// Deocde varint at given pointer and offset, offset is incremented after encoding.
inline uint64_t VarintDecode(void * src, uint64_t & offset)
{
  uint64_t n = 0;
  int shift = 0;
  while (1)
  {
    uint8_t b = *(((uint8_t*)src) + offset);
    CHECK_LESS_OR_EQUAL(shift, 56, ());
    n |= uint64_t(b & 0x7F) << shift;
    ++offset;
    if ((b & 0x80) == 0) break;
    shift += 7;
  }
  return n;
}
// Decode varint using bytes Reader, offset is incremented after decoding.
inline uint64_t VarintDecode(Reader & reader, uint64_t & offset)
{
  uint64_t n = 0;
  int shift = 0;
  while (1)
  {
    uint8_t b = 0;
    reader.Read(offset, &b, 1);
    CHECK_LESS_OR_EQUAL(shift, 56, ());
    n |= uint64_t(b & 0x7F) << shift;
    ++offset;
    if ((b & 0x80) == 0) break;
    shift += 7;
  }
  return n;
}
// Reverse decode varint. Offset should point to last byte of decoded varint.
// It is compulsory that there is at least one encoded varint before this varint.
// After decoding offset points to the last byte of previous varint.
inline uint64_t VarintDecodeReverse(Reader & reader, uint64_t & offset)
{
  uint8_t b = 0;
  do
  {
    --offset;
    reader.Read(offset, &b, 1);
  }
  while ((b & 0x80) != 0);
  uint64_t prevLastEncodedByteOffset = offset;
  ++offset;
  uint64_t num = VarintDecode(reader, offset);
  offset = prevLastEncodedByteOffset;
  return num;
}
