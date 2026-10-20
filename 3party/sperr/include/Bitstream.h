#ifndef BITSTREAM_H
#define BITSTREAM_H

/*
 * Bitstream is intended and optimized for streaming reads and writes.
 *   It is heavily modeled after the bitstream structure in ZFP:
 *   (https://github.com/LLNL/zfp/blob/develop/include/zfp/bitstream.inl)
 *   For heavy use of random reads and writes, please use the Bitmask class instead.
 *
 * Bitstream uses words of 64 bits in its storage. A buffered word of 64 bits is also
 *   used to speed up stream reads and writes.
 *
 * A few caveats:
 *   1. Random reads CAN be achieved via repeated rseek() and rbit() calls.
 *      However, it will be much less efficient than the random reads in Bitmask.
 *   2. A function call of wseek() will erase the remaining bits in a buffered word, i.e.,
 *      from the wseek() position to the next word boundary, though the bits up to the wseek()
 *      position will be preserved. This design is for better efficiency of wbit().
 *   3. Because of 2, true random writes is not possible; it's only possible at the end of
 *      each word, e.g., positions of 63, 127, 191.
 *   4. A function call of flush() will align the writing position to the beginning of the
 *      next word, i.e., the number of truly useful bits is lost!
 *      One wants to call wtell() to retrieve and keep that info.
 *   5. Functions write_bitstream() and parse_bitstream() take in a raw pointer and the
 *      number of bits to write/read. The memory pointed to by the raw pointer needs to
 *      be big enough to hold the number of bits specified.
 *   6. get_bitstream() and write_bitstream() need to be supplied a number of bits because
 *      a Bitstream itself will lose track of how many useful bits are there after flush().
 *   7. Unlike std::vector, a bitstream does NOT have an equivalent concept of "size."
 *      Thus, capacity change brought by `reserve()` can be immediately used to read/write.
 */

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sperr {

class Bitstream {
 public:
  // Constructor
  //
  Bitstream(size_t nbits = 0);  // How many bits does it hold initially?

  // Functions for both read and write
  //
  void rewind();
  auto capacity() const -> size_t;
  void reserve(size_t nbits);
  void reset();  // Reset the bitstream to be all 0's.

  // Functions for read
  //
  auto rtell() const -> size_t;
  void rseek(size_t offset);
  auto rbit() -> bool;

  // Functions for write
  //
  auto wtell() const -> size_t;
  void wseek(size_t offset);
  void wbit(bool bit);
  void flush();

  // Functions that provide or parse a compact bitstream
  //
  void write_bitstream(void* p, size_t num_bits) const;
  void parse_bitstream(const void* p, size_t num_bits);
  auto get_bitstream(size_t num_bits) const -> std::vector<std::byte>;

 private:
  uint64_t m_buffer = 0;  // incoming/outgoing bits
  size_t m_bits = 0;      // number of buffered bits

  std::vector<uint64_t>::iterator m_itr;  // Iterator to the next word to be read/written.
  std::vector<uint64_t> m_buf;
};

}  // namespace sperr

#endif
