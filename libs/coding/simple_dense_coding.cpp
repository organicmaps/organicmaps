#include "coding/simple_dense_coding.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>

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

SimpleDenseCoding::SimpleDenseCoding(std::vector<uint8_t> const & data)
{
  size_t constexpr kAlphabetSize = size_t(std::numeric_limits<uint8_t>::max()) + 1;

  uint64_t frequency[kAlphabetSize] = {0};  // Maps symbols to frequences.
  for (uint8_t symbol : data)
    ++frequency[symbol];

  uint8_t symbols[kAlphabetSize];  // Maps ranks to symbols.
  uint8_t rank[kAlphabetSize];     // Maps symbols to ranks.

  for (size_t i = 0; i < kAlphabetSize; ++i)
    symbols[i] = i;

  auto frequencyCmp = [&frequency](uint8_t lsym, uint8_t rsym) { return frequency[lsym] > frequency[rsym]; };
  std::sort(symbols, symbols + kAlphabetSize, frequencyCmp);
  for (size_t r = 0; r < kAlphabetSize; ++r)
    rank[symbols[r]] = r;

  auto getRank = [&rank](uint8_t sym) { return rank[sym]; };

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
  return m_symbols[m_ranks[static_cast<size_t>(i)]];
}
}  // namespace coding
