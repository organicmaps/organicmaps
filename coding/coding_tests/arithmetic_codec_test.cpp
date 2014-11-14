#include "../arithmetic_codec.hpp"
#include "../reader.hpp"

#include "../../testing/testing.hpp"
#include "../../base/pseudo_random.hpp"

UNIT_TEST(ArithmeticCodec) {
  PseudoRNG32 rng;

  uint32_t const c_max_freq = 2048;
  uint32_t const c_alphabet_size = 256;
  vector<uint32_t> symbols;
  vector<uint32_t> freqs;
  // Generate random freqs.
  for (uint32_t i = 0; i < c_alphabet_size; ++i) {
    uint32_t freq = rng.Generate() % c_max_freq;
    freqs.push_back(freq);
  }
  // Make at least one frequency zero for corner cases.
  freqs[freqs.size() / 2] = 0;
  // Generate symbols based on given freqs.
  for (uint32_t i = 0; i < freqs.size(); ++i) {
    uint32_t freq = freqs[i];
    for (uint32_t j = 0; j < freq; ++j) {
      uint32_t pos = rng.Generate() % (symbols.size() + 1);
      symbols.insert(symbols.begin() + pos, 1, i);
    }
  }
  vector<uint32_t> distr_table = FreqsToDistrTable(freqs);
  // Encode symbols.
  ArithmeticEncoder arith_enc(distr_table);
  for (uint32_t i = 0; i < symbols.size(); ++i) arith_enc.Encode(symbols[i]);
  vector<uint8_t> encoded_data = arith_enc.Finalize();
  // Decode symbols.
  MemReader reader(encoded_data.data(), encoded_data.size());
  ArithmeticDecoder arith_dec(reader, distr_table);
  for (uint32_t i = 0; i < symbols.size(); ++i) {
    uint32_t decoded_symbol = arith_dec.Decode();
    TEST_EQUAL(symbols[i], decoded_symbol, ());
  }
}
