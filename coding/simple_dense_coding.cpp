#include "coding/simple_dense_coding.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/limits.hpp"
#include "std/utility.hpp"

namespace coding
{
namespace
{
size_t const kAlphabetSize = static_cast<size_t>(numeric_limits<uint8_t>::max()) + 1;

struct Code
{
  Code() : m_code(0), m_length(0) {}

  uint8_t m_code;
  uint8_t m_length;
};

// Initializes code table for simple dense coding with following code
// words: 0, 1, 00, 01, 10, 11, 000, 001, ...
struct CodeTable
{
public:
  CodeTable()
  {
    size_t rank = 0;
    uint8_t length = 1;
    while (rank < kAlphabetSize)
    {
      // Number of codes with the same bit length.
      size_t const numCodes = static_cast<size_t>(1) << length;

      uint8_t code = 0;
      for (; code < numCodes && rank + code < kAlphabetSize; ++code)
      {
        size_t const pos = rank + code;
        m_table[pos].m_code = code;
        m_table[pos].m_length = length;
      }

      rank += code;
      length += 1;
    }
  }

  inline Code const & GetCode(uint8_t rank) const { return m_table[rank]; }

private:
  Code m_table[kAlphabetSize];
};

// Calculates frequences for data symbols.
void CalcFrequences(vector<uint8_t> const & data, uint64_t frequency[])
{
  memset(frequency, 0, sizeof(*frequency) * kAlphabetSize);
  for (uint8_t symbol : data)
    ++frequency[symbol];
}
}  // namespace

SimpleDenseCoding::SimpleDenseCoding(vector<uint8_t> const & data)
{
  // This static initialization isn't thread safe prior to C++11.
  static CodeTable codeTable;

  uint64_t frequency[kAlphabetSize];  // Maps symbols to frequences.
  CalcFrequences(data, frequency);

  uint8_t symbols[kAlphabetSize];  // Maps ranks to symbols.
  uint8_t rank[kAlphabetSize];     // Maps symbols to ranks.

  for (size_t i = 0; i < kAlphabetSize; ++i)
    symbols[i] = i;
  sort(symbols, symbols + kAlphabetSize, [&frequency](uint8_t lsym, uint8_t rsym)
  {
    return frequency[lsym] > frequency[rsym];
  });
  for (size_t r = 0; r < kAlphabetSize; ++r)
    rank[symbols[r]] = r;

  uint64_t bitLength = 0;
  for (size_t symbol = 0; symbol < kAlphabetSize; ++symbol)
    bitLength += frequency[symbol] * codeTable.GetCode(rank[symbol]).m_length;

  succinct::bit_vector_builder bitsBuilder;
  bitsBuilder.reserve(bitLength);
  vector<bool> indexBuilder(bitLength);
  size_t pos = 0;
  for (uint8_t symbol : data)
  {
    Code const & code = codeTable.GetCode(rank[symbol]);
    ASSERT_LESS(pos, bitLength, ());
    indexBuilder[pos] = 1;

    bitsBuilder.append_bits(code.m_code, code.m_length);
    pos += code.m_length;
  }
  ASSERT_EQUAL(pos, bitLength, ());

  succinct::bit_vector(&bitsBuilder).swap(m_bits);
  succinct::rs_bit_vector(indexBuilder, true /* with_select_hints */).swap(m_index);
  m_symbols.assign(symbols);
}

SimpleDenseCoding::SimpleDenseCoding(SimpleDenseCoding && rhs) { Swap(move(rhs)); }

uint8_t SimpleDenseCoding::Get(uint64_t i) const
{
  ASSERT_LESS(i, Size(), ());
  uint64_t const start = m_index.select(i);
  uint64_t const end = i + 1 == Size() ? m_index.size() : m_index.select(i + 1);

  ASSERT_LESS(start, end, ());

  uint8_t const length = static_cast<uint8_t>(end - start);
  ASSERT_LESS_OR_EQUAL(length, 8, ());

  uint8_t const code = m_bits.get_bits(start, length);
  uint8_t const rank = (1 << length) - 2 + code;
  return m_symbols[rank];
}
}  // namespace coding
