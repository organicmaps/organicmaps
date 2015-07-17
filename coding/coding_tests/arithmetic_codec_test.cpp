#include "testing/testing.hpp"

#include "coding/arithmetic_codec.hpp"
#include "coding/reader.hpp"

#include "std/random.hpp"

UNIT_TEST(ArithmeticCodec)
{
  mt19937 rng(0);

  uint32_t const MAX_FREQ = 2048;
  uint32_t const ALPHABET_SIZE = 256;
  vector<uint32_t> symbols;
  vector<uint32_t> freqs;
  // Generate random freqs.
  for (uint32_t i = 0; i < ALPHABET_SIZE; ++i) {
    uint32_t freq = rng() % MAX_FREQ;
    freqs.push_back(freq);
  }
  // Make at least one frequency zero for corner cases.
  freqs[freqs.size() / 2] = 0;
  // Generate symbols based on given freqs.
  for (uint32_t i = 0; i < freqs.size(); ++i) {
    uint32_t freq = freqs[i];
    for (uint32_t j = 0; j < freq; ++j) {
      uint32_t pos = rng() % (symbols.size() + 1);
      symbols.insert(symbols.begin() + pos, 1, i);
    }
  }
  vector<uint32_t> distrTable = FreqsToDistrTable(freqs);
  // Encode symbols.
  ArithmeticEncoder arithEnc(distrTable);
  for (uint32_t i = 0; i < symbols.size(); ++i) arithEnc.Encode(symbols[i]);
  vector<uint8_t> encodedData = arithEnc.Finalize();
  // Decode symbols.
  MemReader reader(encodedData.data(), encodedData.size());
  ArithmeticDecoder arithDec(reader, distrTable);
  for (uint32_t i = 0; i < symbols.size(); ++i) {
    uint32_t decodedSymbol = arithDec.Decode();
    TEST_EQUAL(symbols[i], decodedSymbol, ());
  }
}
