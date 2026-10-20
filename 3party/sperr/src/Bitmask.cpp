#include "Bitmask.h"

#include <algorithm>
#include <cassert>
#include <limits>

#if __cplusplus >= 202002L
#include <bit>
#endif

sperr::Bitmask::Bitmask(size_t nbits)
{
  auto num_longs = (nbits + 63) / 64;
  m_buf.assign(num_longs, 0);
  m_num_bits = nbits;
}

auto sperr::Bitmask::size() const -> size_t
{
  return m_num_bits;
}

void sperr::Bitmask::resize(size_t nbits)
{
  auto num_longs = (nbits + 63) / 64;
  m_buf.resize(num_longs, 0);
  m_num_bits = nbits;
}

auto sperr::Bitmask::rlong(size_t idx) const -> uint64_t
{
  return m_buf[idx >> 6];
}

auto sperr::Bitmask::rbit(size_t idx) const -> bool
{
  auto div = idx >> 6;  // idx / 64
  auto rem = idx & 63;  // idx % 64
  auto word = m_buf[div];
  word &= uint64_t{1} << rem;
  return word;
}

auto sperr::Bitmask::has_true(size_t start, size_t len) const -> bool
{
  auto long_idx = start >> 6;
  auto processed_bits = int64_t{0};
  auto word = m_buf[long_idx];

  // Collect the remaining bits from the start long.
  auto begin_idx = start & 63;
  auto nbits = std::min(size_t{64}, begin_idx + len);
  for (auto i = begin_idx; i < nbits; i++) {
    if (word & (uint64_t{1} << i))
      return true;
    processed_bits++;
  }

  // Examine the subsequent full longs.
  while (processed_bits + 64 <= len) {
    word = m_buf[++long_idx];
    if (word) {
      return true;
    }
    processed_bits += 64;
  }

  // Examine the remaining bits
  if (processed_bits < len) {
    nbits = len - processed_bits;
    assert(nbits < 64);
    word = m_buf[++long_idx];
    for (int64_t i = 0; i < nbits; i++) {
      if (word & (uint64_t{1} << i))
        return true;
    }
  }

  return false;
}

auto sperr::Bitmask::find_true(size_t start, size_t len) const -> int64_t
{
  auto long_idx = start >> 6;
  auto processed_bits = int64_t{0};
  auto word = m_buf[long_idx];

  // Collect the remaining bits from the start long.
  auto begin_idx = start & 63;
  auto nbits = std::min(size_t{64}, begin_idx + len);
  for (auto i = begin_idx; i < nbits; i++) {
    if (word & (uint64_t{1} << i))
      return processed_bits;
    processed_bits++;
  }

  // Examine the subsequent full longs.
  while (processed_bits + 64 <= len) {
    word = m_buf[++long_idx];
    if (word) {
#if __cplusplus >= 202002L
      int64_t i = std::countr_zero(word);
      return processed_bits + i;
#else
      for (int64_t i = 0; i < 64; i++)
        if (word & (uint64_t{1} << i))
          return processed_bits + i;
#endif
    }
    processed_bits += 64;
  }

  // Examine the remaining bits
  if (processed_bits < len) {
    nbits = len - processed_bits;
    assert(nbits < 64);
    word = m_buf[++long_idx];
    for (int64_t i = 0; i < nbits; i++) {
      if (word & (uint64_t{1} << i))
        return processed_bits + i;
    }
  }

  return -1;
}

auto sperr::Bitmask::count_true() const -> size_t
{
  size_t counter = 0;
  if (m_buf.empty())
    return counter;

  // Note that unused bits in the last long are not guaranteed to be all 0's.
  for (size_t i = 0; i < m_buf.size() - 1; i++) {
    auto val = m_buf[i];
#if __cplusplus >= 202002L
    counter += std::popcount(val);
#else
    if (val != 0) {
      for (size_t j = 0; j < 64; j++)
        counter += ((val >> j) & uint64_t{1});
    }
#endif
  }
  const auto val = m_buf.back();
  if (val != 0) {
    for (size_t j = 0; j < m_num_bits - (m_buf.size() - 1) * 64; j++)
      counter += ((val >> j) & uint64_t{1});
  }

  return counter;
}

void sperr::Bitmask::wlong(size_t idx, uint64_t value)
{
  m_buf[idx >> 6] = value;
}

void sperr::Bitmask::wbit(size_t idx, bool bit)
{
  const auto wstart = idx >> 6;
  auto word = m_buf[wstart];

  auto mask1 = uint64_t{1} << (idx & 63);
  word &= ~mask1;

  auto mask2 = uint64_t{bit} << (idx & 63);
  word |= mask2;

  m_buf[wstart] = word;
}

void sperr::Bitmask::wtrue(size_t idx)
{
  const auto wstart = idx >> 6;
  const auto mask = uint64_t{1} << (idx & 63);

  auto word = m_buf[wstart];
  word |= mask;
  m_buf[wstart] = word;
}

void sperr::Bitmask::wfalse(size_t idx)
{
  const auto wstart = idx >> 6;
  const auto mask = uint64_t{1} << (idx & 63);

  auto word = m_buf[wstart];
  word &= ~mask;
  m_buf[wstart] = word;
}

void sperr::Bitmask::reset()
{
  std::fill(m_buf.begin(), m_buf.end(), 0);
}

void sperr::Bitmask::reset_true()
{
  std::fill(m_buf.begin(), m_buf.end(), std::numeric_limits<uint64_t>::max());
}

auto sperr::Bitmask::view_buffer() const -> const std::vector<uint64_t>&
{
  return m_buf;
}

void sperr::Bitmask::use_bitstream(const void* p)
{
  const auto* pu64 = static_cast<const uint64_t*>(p);
  std::copy(pu64, pu64 + m_buf.size(), m_buf.begin());
}

#if __cplusplus >= 202002L && defined __cpp_lib_three_way_comparison
auto sperr::Bitmask::operator<=>(const Bitmask& rhs) const noexcept
{
  auto cmp = m_num_bits <=> rhs.m_num_bits;
  if (cmp != 0)
    return cmp;

  if (m_num_bits % 64 == 0)
    return m_buf <=> rhs.m_buf;
  else {
    // Compare each fully used long.
    for (size_t i = 0; i < m_buf.size() - 1; i++) {
      cmp = m_buf[i] <=> rhs.m_buf[i];
      if (cmp != 0)
        return cmp;
    }
    // Compare the last partially used long.
    auto mylast = m_buf.back();
    auto rhslast = rhs.m_buf.back();
    for (size_t i = m_num_bits % 64; i < 64; i++) {
      auto mask = uint64_t{1} << i;
      mylast &= ~mask;
      rhslast &= ~mask;
    }
    return mylast <=> rhslast;
  }
}
auto sperr::Bitmask::operator==(const Bitmask& rhs) const noexcept -> bool
{
  return (operator<=>(rhs) == 0);
}
#endif
