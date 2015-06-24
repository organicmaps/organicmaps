// Author: Artyom Polkovnikov.
// Bits source and sink for sequential read and write of bits.

#pragma once

#include "std/cstdint.hpp"

// Forward declarations.
class Reader;
class Writer;

class BitSink
{
public:
  BitSink(Writer & writer);
  // Note! Last byte is flushed in destructor.
  ~BitSink();
  uint64_t NumBitsWritten() const { return m_size; }
  // Write writeSize number of bits from least significant side of bits number.
  void Write(uint64_t bits, uint32_t writeSize);
private:
  Writer & m_writer;
  uint8_t m_lastByte;
  uint64_t m_size;
  uint64_t m_totalBits;
};

class BitSource
{
public:
  BitSource(Reader & reader);
  uint64_t NumBitsRead() const { return m_totalBitsRead; }
  // Read readSize number of bits, return it as least significant bits of 64-bit number.
  uint64_t Read(uint32_t readSize);
private:
  Reader & m_reader;
  uint64_t m_serialCur;
  uint64_t m_serialEnd;
  uint64_t m_bits;
  uint32_t m_bitsSize;
  uint64_t m_totalBitsRead;
};
