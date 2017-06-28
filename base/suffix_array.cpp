#include "base/suffix_array.hpp"

#include "base/assert.hpp"

#include <limits>

using namespace std;

namespace
{
bool LEQ(size_t a1, size_t a2, size_t b1, size_t b2)
{
  if (a1 != b1)
    return a1 < b1;
  return a2 <= b2;
}

bool LEQ(size_t a1, size_t a2, size_t a3, size_t b1, size_t b2, size_t b3)
{
  if (a1 != b1)
    return a1 < b1;
  return LEQ(a2, a3, b2, b3);
}

template <typename Values>
void RadixSort(size_t n, size_t const * keys, size_t maxValue, Values const & values,
               size_t * resultKeys)
{
  std::vector<size_t> count(maxValue);
  for (size_t i = 0; i < n; ++i)
    ++count[values[keys[i]]];
  for (size_t i = 1; i < maxValue; ++i)
    count[i] += count[i - 1];
  for (size_t i = n - 1; i < n; --i)
    resultKeys[--count[values[keys[i]]]] = keys[i];
}

bool InLeftHalf(size_t n0, size_t pos) { return pos < n0; }

size_t RestoreIndex(size_t n0, size_t pos)
{
  return InLeftHalf(n0, pos) ? pos * 3 + 1 : (pos - n0) * 3 + 2;
}

struct SkewWrapper
{
  SkewWrapper(size_t n, uint8_t const * s) : m_n(n), m_s(s) {}
  size_t size() const { return m_n; }
  size_t operator[](size_t i) const
  {
    if (i < m_n)
      return static_cast<size_t>(m_s[i]) + 1;
    ASSERT_LESS(i, m_n + 3, ());
    return 0;
  }

  size_t const m_n;
  uint8_t const * const m_s;
};

template <typename Container>
struct Slice
{
  Slice(Container const & c, size_t n, size_t offset) : m_c(c), m_n(n), m_offset(offset) {}
  size_t operator[](size_t i) const { return m_c[i + m_offset]; }
  const Container & m_c;
  const size_t m_n;
  const size_t m_offset;
};

template <typename Container>
Slice<Container> MakeSlice(Container const & c, size_t offset)
{
  return Slice<Container>(c, c.size(), offset);
}

// Builds suffix array over the string s, where for all i < n: 0 < s[i] <= k.
//
// Result is written to the array |SA|, where SA[i] is the offset of
// the i-th ranked suffix.
//
// For implementation simplicity, it's assumed that s[n] = s[n + 1] = s[n + 2] =
// 0.
//
// Idea and implementation was inspired by "Simple Linear Work Suffix
// Array Construction" by Juha K¨arkk¨ainen and Peter Sanders.
template <typename S>
void RawSkew(size_t n, size_t maxValue, S const & s, size_t * sa)
{
  size_t const kInvalidId = std::numeric_limits<size_t>::max();

  if (n == 0)
    return;

  if (n == 1)
  {
    sa[0] = 0;
    return;
  }

  // The number of =1 (mod 3) suffixes is the same as the number of =0
  // (mod 3) suffixes.
  const size_t n0 = (n + 2) / 3;  // Number of =0 (mod 3) suffixes.
  const size_t n1 = (n + 1) / 3;  // Number of =1 (mod 3) suffixes.
  const size_t n2 = n / 3;        // Number of =2 (mod 3) suffixes.

  const size_t n02 = n0 + n2;

  const bool fake1 = n0 != n1;
  ASSERT_EQUAL(n1 + fake1, n0, ());
  ASSERT_EQUAL(fake1, (n % 3 == 1), ());

  // Generate positions of =(1|2) (mod 3) suffixes.
  std::vector<size_t> s12(n02 + 3);
  std::vector<size_t> sa12(n02 + 3);

  // (n0 - n1) is needed in case when n == 0 (mod 3).  We need a fake
  // =1 (mod 3) suffix for proper sorting of =0 (mod 3) suffixes.
  // Therefore we force here that the number of =1 (mod 3) suffixes
  // should be the same as the number of =0 (mod 3) suffixes. That's
  // why we need that s[n] = s[n + 1] = s[n + 2] = 0.
  for (size_t i = 0, j = 0; i < n + fake1; ++i)
  {
    if (i % 3 != 0)
      s12[j++] = i;
  }

  RadixSort(n02, s12.data(), maxValue + 1, MakeSlice(s, 2), sa12.data());
  RadixSort(n02, sa12.data(), maxValue + 1, MakeSlice(s, 1), s12.data());
  RadixSort(n02, s12.data(), maxValue + 1, s, sa12.data());

  // Generate lexicographic names for all =(1|2) (mod 3) triples.
  size_t name = 0;
  size_t c0 = kInvalidId;
  size_t c1 = kInvalidId;
  size_t c2 = kInvalidId;
  for (size_t i = 0; i < n02; ++i)
  {
    auto const pos = sa12[i];
    if (s[pos] != c0 || s[pos + 1] != c1 || s[pos + 2] != c2)
    {
      c0 = s[pos];
      c1 = s[pos + 1];
      c2 = s[pos + 2];
      ++name;
    }

    // Puts all =1 (mod 3) suffixes to the left part of s12, puts all
    // =2 (mod 3) suffixes to the right part.
    if (pos % 3 == 1)
      s12[pos / 3] = name;
    else
      s12[pos / 3 + n0] = name;
  }

  if (name < n02)
  {
    // When not all triples unique, we need to build a suffix array
    // for them.
    RawSkew(n02, name, s12, sa12.data());
    for (size_t i = 0; i < n02; ++i)
      s12[sa12[i]] = i + 1;
  }
  else
  {
    // When all triples are unique, it's easy to build a suffix array.
    for (size_t i = 0; i < n02; ++i)
      sa12[s12[i] - 1] = i;
  }

  // SA12 is the suffix array for the string s12 now, and all symbols
  // in s12 are unique.

  // Need to do a stable sort for all =0 (mod 3) suffixes.
  std::vector<size_t> s0(n0);
  std::vector<size_t> sa0(n0);
  for (size_t i = 0, j = 0; i < n02; ++i)
  {
    if (sa12[i] < n0)
      s0[j++] = 3 * sa12[i];
  }

  // s0 is pre-sorted now in accordance with their tails (=1 (mod 3)
  // suffixes). For full sorting we need to do a stable sort =0 (mod
  // 3) suffixes in accordance with their first characters.
  RadixSort(n0, s0.data(), maxValue + 1, s, sa0.data());

  // SA0 is the suffix array for the string s0 now, and all symbols in
  // s0 are unique.

  // Okay, need to merge sorted =0 (mod 3) suffixes and =(1|2) (mod 3)
  // suffixes.
  size_t i0 = 0;
  size_t i12 = fake1;
  size_t k = 0;
  while (i12 != n02 && i0 != n0)
  {
    const size_t p0 = sa0[i0];
    const size_t p12 = RestoreIndex(n0, sa12[i12]);
    ASSERT_LESS(p12 / 3, n0, ());

    if (InLeftHalf(n0, sa12[i12]))
    {
      if (LEQ(s[p12], s12[sa12[i12] + n0], s[p0], s12[p0 / 3]))
      {
        // Suffix =(1|2) (mod 3) is smaller.
        sa[k++] = p12;
        ++i12;
      }
      else
      {
        sa[k++] = p0;
        ++i0;
      }
    }
    else
    {
      if (LEQ(s[p12], s[p12 + 1], s12[sa12[i12] - n0 + 1], s[p0], s[p0 + 1], s12[p0 / 3 + n0]))
      {
        // Suffix =(1|2) (mod 3) is smaller.
        sa[k++] = p12;
        ++i12;
      }
      else
      {
        sa[k++] = p0;
        ++i0;
      }
    }
  }
  for (; i12 != n02; ++k, ++i12)
    sa[k] = RestoreIndex(n0, sa12[i12]);
  for (; i0 != n0; ++k, ++i0)
    sa[k] = sa0[i0];
  ASSERT_EQUAL(k, n, ());
}
}  // namespace

namespace base
{
void Skew(size_t n, uint8_t const * s, size_t * sa)
{
  RawSkew(n, 0xFF /* maxValue */, SkewWrapper(n, s), sa);
}

void Skew(std::string const & s, std::vector<size_t> & sa)
{
  auto const n = s.size();
  sa.assign(n, 0);
  Skew(n, reinterpret_cast<const uint8_t *>(s.data()), sa.data());
}
}  // namespace base
