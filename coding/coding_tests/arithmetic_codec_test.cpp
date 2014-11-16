#include "../arithmetic_codec.hpp"
#include "../reader.hpp"

#include "../../testing/testing.hpp"
#include "../../base/pseudo_random.hpp"

UNIT_TEST(ArithmeticCodec) {
  PseudoRNG32 rng;

  u32 const MAX_FREQ = 2048;
  u32 const ALPHABET_SIZE = 256;
  vector<u32> symbols;
  vector<u32> freqs;
  // Generate random freqs.
  for (u32 i = 0; i < ALPHABET_SIZE; ++i) {
    u32 freq = rng.Generate() % MAX_FREQ;
    freqs.push_back(freq);
  }
  // Make at least one frequency zero for corner cases.
  freqs[freqs.size() / 2] = 0;
  // Generate symbols based on given freqs.
  for (u32 i = 0; i < freqs.size(); ++i) {
    u32 freq = freqs[i];
    for (u32 j = 0; j < freq; ++j) {
      u32 pos = rng.Generate() % (symbols.size() + 1);
      symbols.insert(symbols.begin() + pos, 1, i);
    }
  }
  vector<u32> distrTable = FreqsToDistrTable(freqs);
  // Encode symbols.
  ArithmeticEncoder arithEnc(distrTable);
  for (u32 i = 0; i < symbols.size(); ++i) arithEnc.Encode(symbols[i]);
  vector<u8> encodedData = arithEnc.Finalize();
  // Decode symbols.
  MemReader reader(encodedData.data(), encodedData.size());
  ArithmeticDecoder arithDec(reader, distrTable);
  for (u32 i = 0; i < symbols.size(); ++i) {
    u32 decodedSymbol = arithDec.Decode();
    TEST_EQUAL(symbols[i], decodedSymbol, ());
  }
}
