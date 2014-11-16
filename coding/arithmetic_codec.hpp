// Author: Artyom.
// Arithmetic Encoder/Decoder.
// See http://en.wikipedia.org/wiki/Arithmetic_coding
// Usage:
//   // Compute freqs table by counting number of occurancies of each symbol.
//   // Freqs table should have size equal to number of symbols in the alphabet.
//   // Convert freqs table to distr table.
//   vector<uint32_t> distrTable = FreqsToDistrTable(freqs);
//   ArithmeticEncoder arith_enc(distrTable);
//   // Encode any number of symbols.
//   arith_enc.Encode(10); arith_enc.Encode(17); arith_enc.Encode(0); arith_enc.Encode(4);
//   // Get encoded bytes.
//   vector<uint8_t> encoded_data = arith_enc.Finalize();
//   // Decode encoded bytes. Number of symbols should be provided outside.
//   MemReader reader(encoded_data.data(), encoded_data.size());
//   ArithmeticDecoder arith_dec(&reader, distrTable);
//   uint32_t sym1 = arith_dec.Decode(); uint32_t sym2 = arith_dec.Decode();
//   uint32_t sym3 = arith_dec.Decode(); uint32_t sym4 = arith_dec.Decode();

#pragma once

#include "../std/stdint.hpp"
#include "../std/vector.hpp"

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

// Forward declarations.
class Reader;

// Default shift of distribution table, i.e. all distribution table frequencies are
// normalized by this shift, i.e. distr table upper bound equals (1 << DISTR_SHIFT).
u32 const DISTR_SHIFT = 16;
// Converts symbols frequencies table to distribution table, used in Arithmetic codecs.
vector<u32> FreqsToDistrTable(vector<u32> const & freqs);

class ArithmeticEncoder
{
public:
  // Provided distribution table.
  ArithmeticEncoder(vector<u32> const & distrTable);
  // Encode symbol using given distribution table and add that symbol to output.
  void Encode(u32 symbol);
  // Finalize encoding, flushes remaining bytes from the buffer to output.
  // Returns output vector of encoded bytes.
  vector<u8> Finalize();
private:
  // Propagates carry in case of overflow.
  void PropagateCarry();
private:
  u32 m_begin;
  u32 m_size;
  vector<u8> m_output;
  vector<u32> const & m_distrTable;
};

class ArithmeticDecoder
{
public:
  // Decoder is given a reader to read input bytes,
  // distrTable - distribution table to decode symbols.
  ArithmeticDecoder(Reader & reader, vector<u32> const & distrTable);
  // Decode next symbol from the encoded stream.
  u32 Decode();
private:
  // Read next code byte from encoded stream.
  u8 ReadCodeByte();
private:
  // Current most significant part of code value.
  u32 m_codeValue;
  // Current interval size.
  u32 m_size;
  // Reader and two bounds of encoded data within this Reader.
  Reader & m_reader;
  u64 m_serialCur;
  u64 m_serialEnd;
  
  vector<u32> const & m_distrTable;
};
