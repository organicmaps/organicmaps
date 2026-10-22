#include "SPECK_INT.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

#if __cplusplus >= 202002L
#include <bit>
#endif

//
// Free-standing helper function
//
auto sperr::speck_int_get_num_bitplanes(const void* buf) -> uint8_t
{
  // Given the header definition, directly retrieve the value stored in the first byte.
  const auto* const ptr = static_cast<const uint8_t*>(buf);
  return ptr[0];
}

template <typename T>
sperr::SPECK_INT<T>::SPECK_INT()
{
  static_assert(std::is_integral_v<T>);
  static_assert(std::is_unsigned_v<T>);
}

template <typename T>
auto sperr::SPECK_INT<T>::integer_len() const -> size_t
{
  if constexpr (std::is_same_v<uint64_t, T>)
    return sizeof(uint64_t);
  else if constexpr (std::is_same_v<uint32_t, T>)
    return sizeof(uint32_t);
  else if constexpr (std::is_same_v<uint16_t, T>)
    return sizeof(uint16_t);
  else
    return sizeof(uint8_t);
}

template <typename T>
void sperr::SPECK_INT<T>::set_dims(dims_type dims)
{
  m_dims = dims;
}

template <typename T>
void sperr::SPECK_INT<T>::set_budget(size_t bud)
{
  if (bud == 0)
    m_budget = std::numeric_limits<size_t>::max();
  else {
    while (bud % 8 != 0)
      bud++;
    m_budget = bud;
  }
}

template <typename T>
auto sperr::SPECK_INT<T>::get_speck_num_bits(const void* buf) const -> uint64_t
{
  // Given the header definition, directly retrieve the value stored in bytes 1--9.
  const auto* const ptr = static_cast<const uint8_t*>(buf);
  uint64_t num_bits = 0;
  std::memcpy(&num_bits, ptr + 1, sizeof(num_bits));
  return num_bits;
}

template <typename T>
auto sperr::SPECK_INT<T>::get_stream_full_len(const void* buf) const -> uint64_t
{
  auto num_bits = get_speck_num_bits(buf);
  while (num_bits % 8 != 0)
    ++num_bits;
  return (header_size + num_bits / 8);
}

template <typename T>
void sperr::SPECK_INT<T>::use_bitstream(const void* p, size_t len)
{
  // Header definition: 9 bytes in total:
  // num_bitplanes (uint8_t), num_useful_bits (uint64_t)

  // Step 1: extract num_bitplanes and num_useful_bits
  assert(len >= header_size);
  const auto* const p8 = static_cast<const uint8_t*>(p);
  std::memcpy(&m_num_bitplanes, p8, sizeof(m_num_bitplanes));
  std::memcpy(&m_total_bits, p8 + sizeof(m_num_bitplanes), sizeof(m_total_bits));

  // Step 2: unpack bits.
  //    Note that the bitstream passed in might not be of its original length as a result of
  //    progressive access. In that case, we parse available bits, and pad 0's to make the
  //    bitstream still have `m_total_bits`.
  m_avail_bits = (len - header_size) * 8;
  if (m_avail_bits < m_total_bits) {
    m_bit_buffer.reserve(m_total_bits);
    m_bit_buffer.reset();  // Set buffer to contain all 0's.
    m_bit_buffer.parse_bitstream(p8 + header_size, m_avail_bits);
  }
  else {
    assert(m_avail_bits - m_total_bits < 64);
    m_avail_bits = m_total_bits;
    m_bit_buffer.parse_bitstream(p8 + header_size, m_total_bits);
  }

  // After parsing an incoming bitstream, m_avail_bits <= m_total_bits.
}

template <typename T>
void sperr::SPECK_INT<T>::encode()
{
  m_initialize_lists();
  const auto coeff_len = m_dims[0] * m_dims[1] * m_dims[2];
  m_bit_buffer.reserve(coeff_len);  // A good starting point
  m_bit_buffer.rewind();
  m_total_bits = 0;

  // Mark every coefficient as insignificant
  m_LSP_mask.resize(coeff_len);
  m_LSP_mask.reset();
  m_LSP_new.clear();
  m_LSP_new.reserve(coeff_len / 16);
  m_LIP_mask.resize(coeff_len);
  m_LIP_mask.reset();

  // Treat it as a special case when all coeffs (m_coeff_buf) are zero.
  //    In such a case, we mark `m_num_bitplanes` as zero.
  //    Of course, `m_total_bits` is also zero.
  if (std::all_of(m_coeff_buf.cbegin(), m_coeff_buf.cend(), [](auto v) { return v == 0; })) {
    m_num_bitplanes = 0;
    return;
  }

  // Decide the starting threshold.
  const auto max_coeff = *std::max_element(m_coeff_buf.cbegin(), m_coeff_buf.cend());
  m_num_bitplanes = 1;
  m_threshold = 1;
  // !! Careful loop condition so no integer overflow !!
  while (max_coeff - m_threshold >= m_threshold) {
    m_threshold *= uint_type{2};
    m_num_bitplanes++;
  }

  // Marching over bitplanes.
  for (uint8_t bitplane = 0; bitplane < m_num_bitplanes; bitplane++) {
    m_bitplane_init();
    m_sorting_pass();
    if (m_bit_buffer.wtell() >= m_budget)  // Happens only when fixed-rate compression.
      break;

    m_refinement_pass_encode();
    if (m_bit_buffer.wtell() >= m_budget)  // Happens only when fixed-rate compression.
      break;

    m_threshold /= uint_type{2};
    m_clean_LIS();
  }

  // Record the total number of bits produced, and flush the stream.
  m_total_bits = m_bit_buffer.wtell();
  m_bit_buffer.flush();
}

template <typename T>
void sperr::SPECK_INT<T>::decode()
{
  m_initialize_lists();
  m_bit_buffer.rewind();

  // initialize coefficients to be zero, and sign array to be all positive
  const auto coeff_len = m_dims[0] * m_dims[1] * m_dims[2];
  m_coeff_buf.assign(coeff_len, uint_type{0});
  m_sign_array.resize(coeff_len);
  m_sign_array.reset_true();

  // Mark every coefficient as insignificant.
  m_LSP_mask.resize(coeff_len);
  m_LSP_mask.reset();
  m_LSP_new.clear();
  m_LSP_new.reserve(coeff_len / 16);
  m_LIP_mask.resize(coeff_len);
  m_LIP_mask.reset();

  // Handle the special case of all coeffs (m_coeff_buf) are zero by return now!
  // This case is indicated by both `m_num_bitplanes` and `m_total_bits` equal zero.
  if (m_num_bitplanes == 0) {
    assert(m_total_bits == 0);
    return;
  }

  // Restore the biggest `m_threshold`.
  m_threshold = 1;
  for (uint8_t i = 1; i < m_num_bitplanes; i++)
    m_threshold *= uint_type{2};

  // Marching over bitplanes.
  for (uint8_t bitplane = 0; bitplane < m_num_bitplanes; bitplane++) {
    m_sorting_pass();
    if (m_bit_buffer.rtell() >= m_avail_bits)  // Happens when a partial bitstream is available,
      break;                                   // because of progressive decoding or fixed-rate.

    m_refinement_pass_decode();
    if (m_bit_buffer.rtell() >= m_avail_bits)  // Happens when a partial bitstream is available,
      break;                                   // because of progressive decoding or fixed-rate.

    m_threshold /= uint_type{2};
    m_clean_LIS();
  }

  // The majority of newly identified significant points are initialized by the refinement pass.
  //    However, if the loop breaks after executing the sorting pass, then it leaves newly
  //    identified significant points from this iteration not initialized. We detect this case and
  //    initialize them here. The initialization strategy is the same as in the refinement pass.
  //
  if (!m_LSP_new.empty()) {
    const auto init_val = m_threshold + m_threshold - m_threshold / uint_type{2} - uint_type{1};
    for (auto idx : m_LSP_new)
      m_coeff_buf[idx] = init_val;
  }

  if (m_avail_bits == m_total_bits)
    assert(m_bit_buffer.rtell() == m_total_bits);
  else {
    assert(m_bit_buffer.rtell() >= m_avail_bits);
    assert(m_bit_buffer.rtell() <= m_total_bits);
  }
}

template <typename T>
auto sperr::SPECK_INT<T>::use_coeffs(vecui_type coeffs, Bitmask signs) -> RTNType
{
  if (coeffs.size() != signs.size())
    return RTNType::Error;
  m_coeff_buf = std::move(coeffs);
  m_sign_array = std::move(signs);
  return RTNType::Good;
}

template <typename T>
auto sperr::SPECK_INT<T>::release_coeffs() -> vecui_type&&
{
  return std::move(m_coeff_buf);
}

template <typename T>
auto sperr::SPECK_INT<T>::release_signs() -> Bitmask&&
{
  return std::move(m_sign_array);
}

template <typename T>
auto sperr::SPECK_INT<T>::view_coeffs() const -> const vecui_type&
{
  return m_coeff_buf;
}

template <typename T>
auto sperr::SPECK_INT<T>::view_signs() const -> const Bitmask&
{
  return m_sign_array;
}

template <typename T>
auto sperr::SPECK_INT<T>::encoded_bitstream_len() const -> size_t
{
  // Note that `m_total_bits` and `m_budget` can have 3 comparison outcomes:
  //  1. `m_total_bits < m_budget` no matter whether m_budget is the maximum size_t or not.
  //      In this case, we record all `m_total_bits` bits (simple).
  //  2. `m_total_bits > m_budget`: in this case, we record the value of `m_total_bits` in
  //      the header but only pack `m_budget` bits to the bitstream, creating an equivalence
  //      of truncating the first `m_budget` bits from a bitstream of length `m_total_bits`.
  //  3. `m_total_bits == m_budget`: this case is very unlikely, but if it happens, that's when
  //      `m_budget` happens to be exactly met after a sorting or refinement pass.
  //      In this case, we can also record all `m_total_bits` bits, same as outcome 1.
  //
  // Organic Maps: compare the lengths in uint64_t before narrowing to size_t (32-bit).
  auto bits_to_pack = static_cast<size_t>(std::min<uint64_t>(m_budget, m_total_bits));
  auto bit_in_byte = bits_to_pack / size_t{8};
  if (bits_to_pack % 8 != 0)
    ++bit_in_byte;
  return (header_size + bit_in_byte);
}

template <typename T>
void sperr::SPECK_INT<T>::append_encoded_bitstream(vec8_type& buffer) const
{
  // Step 1: calculate size and allocate space for the encoded bitstream
  //
  // Header definition: 9 bytes in total:
  // num_bitplanes (uint8_t), num_useful_bits (uint64_t)
  //
  const auto app_size = this->encoded_bitstream_len();
  const auto orig_size = buffer.size();
  buffer.resize(orig_size + app_size);
  auto* const ptr = buffer.data() + orig_size;

  // Step 2: fill header
  size_t pos = 0;
  std::memcpy(ptr + pos, &m_num_bitplanes, sizeof(m_num_bitplanes));
  pos += sizeof(m_num_bitplanes);
  std::memcpy(ptr + pos, &m_total_bits, sizeof(m_total_bits));
  pos += sizeof(m_total_bits);

  // Step 3: assemble the right amount of bits into bytes.
  // See discussion on the number of bits to pack in function `encoded_bitstream_len()`.
  // Organic Maps: compare the lengths in uint64_t before narrowing to size_t (32-bit).
  auto bits_to_pack = static_cast<size_t>(std::min<uint64_t>(m_budget, m_total_bits));
  m_bit_buffer.write_bitstream(ptr + header_size, bits_to_pack);
}

template <typename T>
void sperr::SPECK_INT<T>::m_refinement_pass_encode()
{
  // First, process significant pixels previously found.
  //
  const auto tmp1 = std::array<uint_type, 2>{uint_type{0}, m_threshold};
  const auto bits_x64 = m_LSP_mask.size() - m_LSP_mask.size() % 64;

  for (size_t i = 0; i < bits_x64; i += 64) {  // Evaluate 64 bits at a time.
    auto value = m_LSP_mask.rlong(i);

#if __cplusplus >= 202002L
    while (value) {
      auto j = std::countr_zero(value);
      const bool o1 = m_coeff_buf[i + j] >= m_threshold;
      m_coeff_buf[i + j] -= tmp1[o1];
      m_bit_buffer.wbit(o1);
      value &= value - 1;
    }
#else
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          const bool o1 = m_coeff_buf[i + j] >= m_threshold;
          m_coeff_buf[i + j] -= tmp1[o1];
          m_bit_buffer.wbit(o1);
        }
      }
    }
#endif
  }
  for (auto i = bits_x64; i < m_LSP_mask.size(); i++) {  // Evaluate the remaining bits.
    if (m_LSP_mask.rbit(i)) {
      const bool o1 = m_coeff_buf[i] >= m_threshold;
      m_coeff_buf[i] -= tmp1[o1];
      m_bit_buffer.wbit(o1);
    }
  }

  // Second, deal with newly found significant pixels in `m_LSP_mask`.
  // For 1D and 2D cases, it simply means marking those locations.
  // For 3D cases, `m_refinement_extra()` actually refines `m_coeff_buf` values.
  //
  m_refinement_extra();
  for (auto idx : m_LSP_new)
    m_LSP_mask.wtrue(idx);
  m_LSP_new.clear();
}

template <typename T>
void sperr::SPECK_INT<T>::m_refinement_pass_decode()
{
  // Organic Maps: a complete stream contains every refinement bit in this pass.
  // Prefix/progressive streams keep the per-read exhaustion checks in the original path below.
  // The caller still stops at completed sorting/refinement pass boundaries as before.
  if (m_avail_bits == m_total_bits) {
    const auto bits_x64 = m_LSP_mask.size() - m_LSP_mask.size() % 64;
    auto visit = [&](auto&& update) {
      for (size_t i = 0; i < bits_x64; i += 64) {
        auto value = m_LSP_mask.rlong(i);
        while (value) {
          auto j = std::countr_zero(value);
          update(i + j);
          value &= value - 1;
        }
      }
      for (auto i = bits_x64; i < m_LSP_mask.size(); i++)
        if (m_LSP_mask.rbit(i)) update(i);
    };
    if (m_threshold >= uint_type{2}) {
      const auto half_t = m_threshold / uint_type{2};
      visit([&](size_t idx) { m_coeff_buf[idx] += m_bit_buffer.rbit() ? half_t : uint_type{0} - half_t; });
    }
    else
      visit([&](size_t idx) { m_coeff_buf[idx] += m_bit_buffer.rbit(); });
    const auto init_val = m_threshold + m_threshold - m_threshold / uint_type{2} - uint_type{1};
    for (auto idx : m_LSP_new) m_coeff_buf[idx] = init_val;
    for (auto idx : m_LSP_new) m_LSP_mask.wtrue(idx);
    m_LSP_new.clear();
    return;
  }

  // First, process significant points previously found.
  //    All these nested conditions are a little annoying, but I don't have a better solution now.
  //    Here's a documentation of their purposes.
  // 1) The decoding scheme (reconstructing values at the middle of an interval) requires
  //    different treatment when `m_threshold` is 1 or not.
  // 2) We make use of the internal representation of `m_LSP_mask` and evaluate 64 bits at time.
  //    This requires evaluating any remaining bits not divisible by 64.
  // 3) During progressive or fixed-rate decoding, we need to evaluate if the bitstream is
  //    exhausted after every read. We test it no matter what decoding mode we're in though.
  // 4) goto is used again. Here's it's used to jump out of a nested loop, which is an endorsed
  //    usage of it: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-goto
  //
  auto read_pos = m_bit_buffer.rtell();  // Avoid repeated calls to rtell().
  const auto bits_x64 = m_LSP_mask.size() - m_LSP_mask.size() % 64;  // <-- Point 2
  if (m_threshold >= uint_type{2}) {                                 // <-- Point 1
    const auto half_t = m_threshold / uint_type{2};
    for (size_t i = 0; i < bits_x64; i += 64) {  // <-- Point 2
      auto value = m_LSP_mask.rlong(i);

#if __cplusplus >= 202002L
      while (value) {
        auto j = std::countr_zero(value);
        if (m_bit_buffer.rbit())
          m_coeff_buf[i + j] += half_t;
        else
          m_coeff_buf[i + j] -= half_t;
        if (++read_pos == m_avail_bits)              // <-- Point 3
          goto INITIALIZE_NEWLY_FOUND_POINTS_LABEL;  // <-- Point 4
        value &= value - 1;
      }
#else
      if (value != 0) {
        for (size_t j = 0; j < 64; j++) {
          if ((value >> j) & uint64_t{1}) {
            if (m_bit_buffer.rbit())
              m_coeff_buf[i + j] += half_t;
            else
              m_coeff_buf[i + j] -= half_t;
            if (++read_pos == m_avail_bits)              // <-- Point 3
              goto INITIALIZE_NEWLY_FOUND_POINTS_LABEL;  // <-- Point 4
          }
        }
      }
#endif
    }
    for (auto i = bits_x64; i < m_LSP_mask.size(); i++) {  // <-- Point 2
      if (m_LSP_mask.rbit(i)) {
        if (m_bit_buffer.rbit())
          m_coeff_buf[i] += half_t;
        else
          m_coeff_buf[i] -= half_t;
        if (++read_pos == m_avail_bits)              // <-- Point 3
          goto INITIALIZE_NEWLY_FOUND_POINTS_LABEL;  // <-- Point 4
      }
    }
  }  // Finish the case where `m_threshold >= 2`.
  else {  // Start the case where `m_threshold == 1`.
    for (size_t i = 0; i < bits_x64; i += 64) {
      auto value = m_LSP_mask.rlong(i);

#if __cplusplus >= 202002L
      while (value) {
        auto j = std::countr_zero(value);
        if (m_bit_buffer.rbit())
          ++(m_coeff_buf[i + j]);
        if (++read_pos == m_avail_bits)
          goto INITIALIZE_NEWLY_FOUND_POINTS_LABEL;
        value &= value - 1;
      }
#else
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          if (m_bit_buffer.rbit())
            ++(m_coeff_buf[i + j]);
          if (++read_pos == m_avail_bits)
            goto INITIALIZE_NEWLY_FOUND_POINTS_LABEL;
        }
      }
#endif
    }
    for (auto i = bits_x64; i < m_LSP_mask.size(); i++) {
      if (m_LSP_mask.rbit(i)) {
        if (m_bit_buffer.rbit())
          ++(m_coeff_buf[i]);
        if (++read_pos == m_avail_bits)
          goto INITIALIZE_NEWLY_FOUND_POINTS_LABEL;
      }
    }
  }
  assert(m_bit_buffer.rtell() <= m_avail_bits);

  // Second, initialize newly found significant points. Here I aim to initialize the reconstructed
  //    value at the middle of the interval specified by `m_threshold`. Note that given the integer
  //    nature of these coefficients, there are actually two values equally "in the middle."
  //    For example, with `m_threshold == 4`, the interval is [4, 8), and both 5 and 6 are "in the
  //    middle." I choose the smaller one (5 in this example) here. My experiments show that
  //    choosing the smaller value rather than the bigger one does not hurt, and sometimes bring a
  //    little extra PSNR gain (<0.5). Also note that the formula calculating `init_val`
  //    makes sure that when `m_threshold == 1`, significant points are initialized as 1.
  //
INITIALIZE_NEWLY_FOUND_POINTS_LABEL:
  const auto init_val = m_threshold + m_threshold - m_threshold / uint_type{2} - uint_type{1};
  for (auto idx : m_LSP_new)
    m_coeff_buf[idx] = init_val;
  for (auto idx : m_LSP_new)
    m_LSP_mask.wtrue(idx);
  m_LSP_new.clear();
}

template class sperr::SPECK_INT<uint64_t>;
template class sperr::SPECK_INT<uint32_t>;
template class sperr::SPECK_INT<uint16_t>;
template class sperr::SPECK_INT<uint8_t>;
