#ifndef BIT_BUFFER_H
#define BIT_BUFFER_H

//
// This class mimics basic functionalities of a std::vector<bool>.
// It provides two important additions though:
// 1) the capability to dump its content as bytes, and
// 2) the capability to take in a chunk of raw memory and interpret
//    it as booleans.
//
// Note1:  why not using a vector of std::bitset?
//         Because its method `to_ulong` throws, not very performance oriented.
// Note 2: This class only works on little endian machines (99.9% of all
// machines).
//

#include <cstddef>  // size_t
#include <cstdint>  // fixed width integers
#include <cstdlib>
#include <vector>

namespace speck {

class bit_buffer {
 public:
  // Same behavior as in a std::vector
  void reserve(size_t);
  void clear() noexcept;
  auto empty() const noexcept -> bool;
  auto size() const noexcept -> size_t;

  // Same usage as a std::vector, slightly different specification here.
  void push_back(bool);
  auto data() const noexcept -> const uint8_t*;

  // How many bytes should you take from the data() method
  auto data_size() const noexcept -> size_t;

  // Serial and parallel data access methods
  auto peek(size_t) const -> bool;
  auto par_peek(size_t) const -> bool;

  // Populate this bit_buffer using a chunk of raw bytes.
  // Existing content of this bit_buffer will be destroyed.
  // It returns success/fail
  auto populate(const void* memory, size_t mem_len, size_t num_bits) -> bool;

 private:
  mutable std::vector<uint8_t> m_vec;
  size_t m_total_bits = 0;

  // Choose uint8_t instead of bool to serve as cache, because
  // 1) bool isn't guaranteed to be 1 byte, and
  // 2) C++ standard guarantees conversions between bools and integers:
  //    false <--> 0, true <--> 1.
  mutable uint8_t m_cache[8];

  // How full is m_cache? [0, 8]
  mutable size_t m_cache_full = 0;

  // Which position in m_vec should m_cache be pointing at.
  // When m_cache_vec_pos < m_vec.size(), m_cache keeps a byte from m_vec
  // unpacked. When m_cache_vec_pos == m_vec.size(), m_cache keeps 8 booleans to
  // be packed. When m_cache_vec_pos > m_vec.size(), something is wrong.
  mutable size_t m_cache_vec_pos = 0;

  // Flush content of m_cache to m_vec if the content isn't in m_vec yet.
  void m_flush_cache() const;

  const uint64_t MAGIC = 0x8040201008040201;
  const uint64_t MASK = 0x8080808080808080;

};  // end of class bvec

}  // end of namespace speck

#endif
