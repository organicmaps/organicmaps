#include "coding/bwt.hpp"

#include "base/assert.hpp"
#include "base/suffix_array.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace
{
size_t const kNumBytes = 256;

// Fake trailing '$' for the BWT, used for original string
// reconstruction.
uint32_t const kEOS = 256;

// FirstColumn represents the first column in the BWT matrix. As
// during reverse BWT we need to reconstruct canonical first column,
// with '$' as the first element, this wrapper is used. Also note that
// other characters in the first column are sorted, so we actually
// don't need to store them explicitly, it's enough to store start
// positions of the corresponding groups of consecutive characters.
class FirstColumn
{
public:
  FirstColumn(size_t n, uint8_t const * s) : m_n(n), m_starts({})
  {
    m_starts.fill(0);
    for (size_t i = 0; i < n; ++i)
      ++m_starts[s[i]];

    size_t offset = 0;
    for (size_t i = 0; i < m_starts.size(); ++i)
    {
      auto const count = m_starts[i];
      m_starts[i] = offset;
      offset += count;
    }
  }

  size_t Size() const { return m_n + 1; }

  uint32_t operator[](size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    if (i == 0)
      return kEOS;

    --i;
    auto it = std::upper_bound(m_starts.begin(), m_starts.end(), i);
    ASSERT(it != m_starts.begin(), ());
    --it;
    return static_cast<uint32_t>(std::distance(m_starts.begin(), it));
  }

  // Returns the rank of the i-th symbol among symbols with the same
  // value.
  size_t Rank(size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    if (i == 0)
      return 0;

    --i;
    auto it = std::upper_bound(m_starts.begin(), m_starts.end(), i);
    if (it == m_starts.begin())
      return i;
    --it;
    return i - *it;
  }

private:
  size_t const m_n;
  std::array<size_t, kNumBytes> m_starts;
};

// LastColumn represents the last column in the BWT matrix. As during
// reverse BWT we need to reconstruct canonical last column, |s| is
// replaced by s[start] + s[0, start) + '$' + s[start, n).
class LastColumn
{
public:
  LastColumn(size_t n, size_t start, uint8_t const * s) : m_n(n), m_start(start), m_s(s)
  {
    for (size_t i = 0; i < Size(); ++i)
    {
      auto const b = (*this)[i];
      if (b == kEOS)
        continue;
      ASSERT_LESS(b, kNumBytes, ());
      m_table[b].push_back(i);
    }
  }

  size_t Size() const { return m_n + 1; }

  uint32_t operator[](size_t i) const
  {
    if (i == 0)
    {
      ASSERT_LESS(m_start, m_n, ());
      return m_s[m_start];
    }

    if (i == m_start + 1)
      return kEOS;

    ASSERT_LESS_OR_EQUAL(i, m_n, ());
    return m_s[i - 1];
  }

  // Returns the index of the |rank|-th |byte| in the canonical BWT
  // last column.
  size_t Select(uint32_t byte, size_t rank)
  {
    if (byte == kEOS)
    {
      ASSERT_EQUAL(rank, 0, ());
      return 0;
    }

    ASSERT_LESS(rank, m_table[byte].size(), (byte, rank));
    return m_table[byte][rank];
  }

private:
  size_t const m_n;
  size_t const m_start;
  uint8_t const * const m_s;
  std::array<std::vector<size_t>, kNumBytes> m_table;
};
}  // namespace

namespace coding
{
size_t BWT(size_t n, uint8_t const * s, uint8_t * r)
{
  std::vector<size_t> sa(n);
  base::Skew(n, s, sa.data());

  size_t result = 0;
  for (size_t i = 0; i < n; ++i)
  {
    if (sa[i] != 0)
    {
      r[i] = s[sa[i] - 1];
    }
    else
    {
      result = i;
      r[i] = s[n - 1];
    }
  }
  return result;
}

size_t BWT(std::string const & s, std::string & r)
{
  auto const n = s.size();
  r.assign(n, '\0');
  return BWT(n, reinterpret_cast<uint8_t const *>(s.data()), reinterpret_cast<uint8_t *>(&r[0]));
}

void RevBWT(size_t n, size_t start, uint8_t const * s, uint8_t * r)
{
  if (n == 0)
    return;

  FirstColumn first(n, s);
  LastColumn last(n, start, s);

  auto curr = start + 1;
  for (size_t i = 0; i < n; ++i)
  {
    ASSERT_LESS(curr, first.Size(), ());
    ASSERT(first[curr] != kEOS, ());

    r[i] = first[curr];
    curr = last.Select(r[i], first.Rank(curr));
  }

  ASSERT_EQUAL(first[curr], kEOS, ());
}

void RevBWT(size_t start, std::string const & s, std::string & r)
{
  auto const n = s.size();
  r.assign(n, '\0');
  RevBWT(n, start, reinterpret_cast<uint8_t const *>(s.data()), reinterpret_cast<uint8_t *>(&r[0]));
}
}  // namespace coding
