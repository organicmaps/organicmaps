#include "bit_buffer.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()

void speck::bit_buffer::reserve(size_t N)
{
  auto num_bytes = N / 8;
  if (N % 8 != 0)
    num_bytes++;
  m_vec.reserve(num_bytes);
}

void speck::bit_buffer::clear() noexcept
{
  m_vec.clear();
  m_total_bits = 0;
  m_cache_full = 0;
  m_cache_vec_pos = 0;
}

auto speck::bit_buffer::empty() const noexcept -> bool
{
  return (m_total_bits == 0);
}

auto speck::bit_buffer::size() const noexcept -> size_t
{
  return m_total_bits;
}

void speck::bit_buffer::m_flush_cache() const
{
  assert(m_cache_vec_pos <= m_vec.size());
  assert(m_cache_full <= 8);

  // Only need to flush if 1) m_cache holds bits and 2) those bits are not in
  // yet.
  if (m_cache_full > 0 && m_cache_vec_pos == m_vec.size()) {
    for (auto i = m_cache_full; i < 8; i++)
      m_cache[i] = 0;
    uint64_t t;
    std::memcpy(&t, m_cache, 8);
    m_vec.emplace_back((MAGIC * t) >> 56);
  }
}

void speck::bit_buffer::push_back(bool val)
{
  // If m_cache_vec_pos is at the middle of m_vec, then try to either unpack the
  // last byte if it's half-full, or manually set it up to point
  // to the end of m_vec.
  if (m_cache_vec_pos != m_vec.size()) {
    if (m_vec.size() * 8 > m_total_bits) {
      // unpack the last byte
      uint64_t t = ((MAGIC * m_vec.back()) & MASK) >> 7;
      std::memcpy(m_cache, &t, 8);
      m_vec.pop_back();
    }

    m_cache_full = m_total_bits - m_vec.size() * 8;
    m_cache_vec_pos = m_vec.size();
  }

  assert(m_cache_full < 8);
  assert(m_cache_vec_pos == m_vec.size());

  m_cache[m_cache_full++] = val;

  if (m_cache_full == 8) {  // Pack m_cache to m_vec
    uint64_t t;
    std::memcpy(&t, m_cache, 8);
    m_vec.emplace_back((MAGIC * t) >> 56);

    m_cache_full = 0;
    m_cache_vec_pos = m_vec.size();
  }

  m_total_bits++;
}

auto speck::bit_buffer::data() const noexcept -> const uint8_t*
{
  m_flush_cache();
  return m_vec.data();
}

auto speck::bit_buffer::data_size() const noexcept -> size_t
{
  auto nbytes = m_total_bits / 8;
  if (m_total_bits % 8 != 0)
    nbytes++;
  return nbytes;
}

auto speck::bit_buffer::peek(size_t idx) const -> bool
{
  auto byte_idx = idx / 8;
  auto cache_idx = idx % 8;

  if (byte_idx == m_cache_vec_pos && cache_idx < m_cache_full) {
    return m_cache[cache_idx];
  }
  else {
    // Need to flush before overwriting content in m_cache!
    m_flush_cache();

    // unpack the correct byte
    uint64_t t = ((MAGIC * m_vec[byte_idx]) & MASK) >> 7;
    std::memcpy(m_cache, &t, 8);

    m_cache_vec_pos = byte_idx;
    m_cache_full = std::min(size_t{8}, m_total_bits - byte_idx * 8);

    return m_cache[cache_idx];
  }
}

auto speck::bit_buffer::par_peek(size_t idx) const -> bool
{
  auto byte_idx = idx / 8;
  auto cache_idx = idx % 8;

  // unpack the correct byte and not worry about m_cache at all
  uint64_t t = ((MAGIC * m_vec[byte_idx]) & MASK) >> 7;
  uint8_t cache[8];
  std::memcpy(cache, &t, 8);
  return cache[cache_idx];
}

auto speck::bit_buffer::populate(const void* memory, size_t mem_len, size_t num_bits) -> bool
{
  auto nbytes = num_bits / 8;
  if (num_bits % 8 != 0)
    nbytes++;
  if (nbytes != mem_len)
    return false;

  this->clear();

  // Step 1: copy over the memory content to m_vec
  const uint8_t* mem_ptr = reinterpret_cast<const uint8_t*>(memory);
  m_vec.resize(mem_len, 0);
  for (size_t i = 0; i < mem_len; i++)
    m_vec[i] = mem_ptr[i];

  // Step 2: set the total number of bits
  m_total_bits = num_bits;

  return true;
}
