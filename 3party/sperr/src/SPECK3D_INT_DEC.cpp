#include "SPECK3D_INT_DEC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

template <typename T>
void sperr::SPECK3D_INT_DEC<T>::m_process_S(size_t idx1, size_t idx2, size_t& counter, bool read)
{
  auto& set = m_LIS[idx1][idx2];

  bool is_sig = true;
  if (read)
    is_sig = m_bit_buffer.rbit();

  if (is_sig) {
    counter++;
    m_code_S(idx1, idx2);
    set.make_empty();  // this current set is gonna be discarded.
  }
}

template <typename T>
void sperr::SPECK3D_INT_DEC<T>::m_process_P(size_t idx, size_t no_use, size_t& counter, bool read)
{
  bool is_sig = true;
  if (read)
    is_sig = m_bit_buffer.rbit();

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_sign_array.wbit(idx, m_bit_buffer.rbit());
    m_LSP_new.push_back(idx);
    m_LIP_mask.wfalse(idx);
  }
}

template <typename T>
void sperr::SPECK3D_INT_DEC<T>::m_process_P_lite(size_t idx)
{
  auto is_sig = m_bit_buffer.rbit();

  if (is_sig) {
    m_sign_array.wbit(idx, m_bit_buffer.rbit());
    m_LSP_new.push_back(idx);
    m_LIP_mask.wfalse(idx);
  }
}

template class sperr::SPECK3D_INT_DEC<uint64_t>;
template class sperr::SPECK3D_INT_DEC<uint32_t>;
template class sperr::SPECK3D_INT_DEC<uint16_t>;
template class sperr::SPECK3D_INT_DEC<uint8_t>;
