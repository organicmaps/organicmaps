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

#include "std/cstdint.hpp"
#include "std/vector.hpp"

// Forward declarations.
class Reader;

// Default shift of distribution table, i.e. all distribution table frequencies are
// normalized by this shift, i.e. distr table upper bound equals (1 << DISTR_SHIFT).
uint32_t const DISTR_SHIFT = 16;
// Converts symbols frequencies table to distribution table, used in Arithmetic codecs.
vector<uint32_t> FreqsToDistrTable(vector<uint32_t> const & freqs);

class ArithmeticEncoder
{
public:
  // Provided distribution table.
  ArithmeticEncoder(vector<uint32_t> const & distrTable);
  // Encode symbol using given distribution table and add that symbol to output.
  void Encode(uint32_t symbol);
  // Finalize encoding, flushes remaining bytes from the buffer to output.
  // Returns output vector of encoded bytes.
  vector<uint8_t> Finalize();
private:
  // Propagates carry in case of overflow.
  void PropagateCarry();
private:
  uint32_t m_begin;
  uint32_t m_size;
  vector<uint8_t> m_output;
  vector<uint32_t> const & m_distrTable;
};

class ArithmeticDecoder
{
public:
  // Decoder is given a reader to read input bytes,
  // distrTable - distribution table to decode symbols.
  ArithmeticDecoder(Reader & reader, vector<uint32_t> const & distrTable);
  // Decode next symbol from the encoded stream.
  uint32_t Decode();
private:
  // Read next code byte from encoded stream.
  uint8_t ReadCodeByte();
private:
  // Current most significant part of code value.
  uint32_t m_codeValue;
  // Current interval size.
  uint32_t m_size;
  // Reader and two bounds of encoded data within this Reader.
  Reader & m_reader;
  uint64_t m_serialCur;
  uint64_t m_serialEnd;
  
  vector<uint32_t> const & m_distrTable;
};
