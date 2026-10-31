#ifndef SPECK_INT_H
#define SPECK_INT_H

//
// This is the base class of 1D, 2D, and 3D integer SPECK implementations.
//

#include "sperr_helper.h"

#include "Bitmask.h"
#include "Bitstream.h"

namespace sperr {

//
// Given a bitstream, one needs to know the integer length needed *before* instantiate a
//    SPECK_INT class to decode the bitstream. Thus, provide this free-standing helper.
//
auto speck_int_get_num_bitplanes(const void* bitstream) -> uint8_t;

//
// Class SPECK_INT
//
template <typename T>
class SPECK_INT {
  using uint_type = T;
  using vecui_type = std::vector<uint_type>;

 public:
  // Constructor and destructor
  SPECK_INT();
  virtual ~SPECK_INT() = default;

  static const size_t header_size = 9;  // 9 bytes

  // The length (1, 2, 4, 8) of the integer type in use
  auto integer_len() const -> size_t;

  // Optional: set a bit budget (num. of bits) for encoding, which is the maximum value of
  // type size_t by default. Passing in zero here resets it to the maximum of size_t.
  void set_budget(size_t);
  void set_dims(dims_type);

  // Note: `speck_int_get_num_bitplanes()` is provided as a free-standing helper function (above).
  //
  // Retrieve the number of useful bits of a SPECK bitstream from its header.
  auto get_speck_num_bits(const void*) const -> uint64_t;
  // Retrieve the number of bytes of a SPECK bitstream (including header) from its header.
  auto get_stream_full_len(const void*) const -> uint64_t;

  // Actions
  void encode();
  void decode();

  // Input
  auto use_coeffs(vecui_type coeffs, Bitmask signs) -> RTNType;
  void use_bitstream(const void* p, size_t len);

  // Output
  auto encoded_bitstream_len() const -> size_t;
  void append_encoded_bitstream(vec8_type& buf) const;
  auto release_coeffs() -> vecui_type&&;
  auto release_signs() -> Bitmask&&;
  auto view_coeffs() const -> const vecui_type&;
  auto view_signs() const -> const Bitmask&;

 protected:
  // Core SPECK procedures
  virtual void m_clean_LIS() = 0;
  virtual void m_sorting_pass() = 0;
  virtual void m_initialize_lists() = 0;
  virtual void m_bitplane_init() {}
  virtual void m_refinement_extra() {}
  void m_refinement_pass_encode();
  void m_refinement_pass_decode();

  // Data members
  uint64_t m_total_bits = 0;  // The number of bits of a complete SPECK stream.
  uint64_t m_avail_bits = 0;  // Decoding only. `m_avail_bits` <= `m_total_bits`
  size_t m_budget = std::numeric_limits<size_t>::max();
  uint_type m_threshold = 0;
  uint8_t m_num_bitplanes = 0;

  dims_type m_dims = {0, 0, 0};
  vecui_type m_coeff_buf;
  std::vector<uint64_t> m_LSP_new;
  Bitmask m_LSP_mask, m_LIP_mask, m_sign_array;
  Bitstream m_bit_buffer;
};

}  // namespace sperr

#endif
