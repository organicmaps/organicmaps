#include "coding/simple_dense_coding.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/limits.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include <boost/range/adaptor/transformed.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace coding
{
namespace
{
size_t const kAlphabetSize = static_cast<size_t>(numeric_limits<uint8_t>::max()) + 1;

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
  uint64_t frequency[kAlphabetSize];  // Maps symbols to frequences.
  CalcFrequences(data, frequency);

  uint8_t symbols[kAlphabetSize];  // Maps ranks to symbols.
  uint8_t rank[kAlphabetSize];     // Maps symbols to ranks.

  for (size_t i = 0; i < kAlphabetSize; ++i)
    symbols[i] = i;

  auto frequencyCmp = [&frequency](uint8_t lsym, uint8_t rsym)
  {
    return frequency[lsym] > frequency[rsym];
  };
  sort(symbols, symbols + kAlphabetSize, frequencyCmp);
  for (size_t r = 0; r < kAlphabetSize; ++r)
    rank[symbols[r]] = r;

  auto getRank = [&rank](uint8_t sym)
  {
    return rank[sym];
  };

  using namespace boost::adaptors;
  succinct::elias_fano_compressed_list(data | transformed(getRank)).swap(m_ranks);
  m_symbols.assign(symbols);
}

SimpleDenseCoding::SimpleDenseCoding(SimpleDenseCoding && rhs)
{
  m_ranks.swap(rhs.m_ranks);
  m_symbols.swap(rhs.m_symbols);
}

uint8_t SimpleDenseCoding::Get(uint64_t i) const
{
  ASSERT_LESS(i, Size(), ());
  return m_symbols[m_ranks[i]];
}
}  // namespace coding
