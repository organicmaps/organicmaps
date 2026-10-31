#include "SPECK2D_INT_ENC.h"

#include <algorithm>
#include <cassert>
#include <numeric>

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_process_S(size_t idx1,
                                            size_t idx2,
                                            size_t& counter,
                                            bool need_decide)
{
  auto& set = m_LIS[idx1][idx2];
  assert(!set.is_pixel());
  bool is_sig = true;

  if (need_decide) {
    is_sig = m_decide_S_significance(set);
    m_bit_buffer.wbit(is_sig);
  }

  if (is_sig) {
    counter++;
    m_code_S(idx1, idx2);
    set.make_empty();
  }
}

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_process_P(size_t idx, size_t& counter, bool need_decide)
{
  bool is_sig = true;

  if (need_decide) {
    is_sig = (m_msb_buf[idx] >= m_msb_threshold);
    m_bit_buffer.wbit(is_sig);
  }

  if (is_sig) {
    counter++;
    m_bit_buffer.wbit(m_sign_array.rbit(idx));
    m_LSP_new.push_back(idx);
    m_LIP_mask.wfalse(idx);
  }
}

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_process_I(bool need_decide)
{
  if (m_I.part_level > 0) {  // Only process `m_I` when it's not empty
    bool is_sig = true;
    if (need_decide) {
      is_sig = m_decide_I_significance();
      m_bit_buffer.wbit(is_sig);
    }

    if (is_sig)
      m_code_I();
  }
}

template <typename T>
auto sperr::SPECK2D_INT_ENC<T>::m_decide_S_significance(const Set2D& set) const -> bool
{
  assert(!set.is_empty());

  for (auto y = set.start_y; y < (set.start_y + set.length_y); y++) {
    auto first = m_msb_buf.cbegin() + y * m_dims[0] + set.start_x;
    if (std::any_of(first, first + set.length_x,
                    [thld = m_msb_threshold](auto v) { return v >= thld; }))
      return true;
  }
  return false;
}

template <typename T>
auto sperr::SPECK2D_INT_ENC<T>::m_decide_I_significance() const -> bool
{
  // First, test the bottom rectangle.
  // It's stored in a contiguous chunk of memory till the buffer end.
  //
  assert(m_I.length_x == m_dims[0]);
  auto first = m_msb_buf.cbegin() + size_t{m_I.start_y} * size_t{m_I.length_x};
  auto len = m_msb_buf.size() - size_t{m_I.start_y} * size_t{m_I.length_x};
  if (std::any_of(first, first + len, [thld = m_msb_threshold](auto v) { return v >= thld; }))
    return true;

  // Second, test the rectangle that's directly to the right of the missing top-left corner.
  //
  len = m_dims[0] - m_I.start_x;
  for (auto y = 0u; y < m_I.start_y; y++) {
    first = m_msb_buf.cbegin() + y * m_dims[0] + m_I.start_x;
    if (std::any_of(first, first + len, [thld = m_msb_threshold](auto v) { return v >= thld; }))
      return true;
  }
  return false;
}

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_additional_initialization()
{
  const auto len = m_dims[0] * m_dims[1] * m_dims[2];
  m_msb_buf.resize(len);
  std::transform(m_coeff_buf.cbegin(), m_coeff_buf.cend(), m_msb_buf.begin(),
                 [](auto v) { return sperr::msb_position(v); });
}

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_bitplane_init()
{
  m_msb_threshold = sperr::msb_position(m_threshold);
}

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_refinement_extra()
{
  for (auto idx : m_LSP_new) {
    assert(m_coeff_buf[idx] >= m_threshold);
    m_coeff_buf[idx] -= m_threshold;
  }
}

template class sperr::SPECK2D_INT_ENC<uint8_t>;
template class sperr::SPECK2D_INT_ENC<uint16_t>;
template class sperr::SPECK2D_INT_ENC<uint32_t>;
template class sperr::SPECK2D_INT_ENC<uint64_t>;
