#include "SPECK1D_INT_DEC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

#if __cplusplus >= 202002L
#include <bit>
#endif

template <typename T>
void sperr::SPECK1D_INT_DEC<T>::m_sorting_pass()
{
  // Since we have a separate representation of LIP, let's process that list first
  //
  const auto bits_x64 = m_LIP_mask.size() - m_LIP_mask.size() % 64;

  for (size_t i = 0; i < bits_x64; i += 64) {
    auto value = m_LIP_mask.rlong(i);

#if __cplusplus >= 202002L
    while (value) {
      size_t j = std::countr_zero(value);
      m_process_P(i + j, j, true);
      value &= value - 1;
    }
#else
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          size_t dummy = 0;
          m_process_P(i + j, dummy, true);
        }
      }
    }
#endif
  }
  for (auto i = bits_x64; i < m_LIP_mask.size(); i++) {
    if (m_LIP_mask.rbit(i)) {
      size_t dummy = 0;
      m_process_P(i, dummy, true);
    }
  }

  // Then we process regular sets in LIS.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    // From the end of m_LIS to its front
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      size_t dummy = 0;
      m_process_S(idx1, idx2, dummy, true);
    }
  }
}

template <typename T>
void sperr::SPECK1D_INT_DEC<T>::m_process_S(size_t idx1, size_t idx2, size_t& counter, bool read)
{
  auto& set = m_LIS[idx1][idx2];
  bool is_sig = true;

  if (read)
    is_sig = m_bit_buffer.rbit();

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_code_S(idx1, idx2);
    set.set_length(0);  // this current set is gonna be discarded.
  }
}

template <typename T>
void sperr::SPECK1D_INT_DEC<T>::m_process_P(size_t idx, size_t& counter, bool read)
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
void sperr::SPECK1D_INT_DEC<T>::m_code_S(size_t idx1, size_t idx2)
{
  auto subsets = m_partition_set(m_LIS[idx1][idx2]);
  auto sig_counter = size_t{0};
  auto read = bool{true};

  // Process the 1st subset
  const auto& set0 = subsets[0];
  assert(set0.get_length() != 0);
  if (set0.get_length() == 1) {
    m_LIP_mask.wtrue(set0.get_start());
    m_process_P(set0.get_start(), sig_counter, read);
  }
  else {
    const auto newidx1 = set0.get_level();
    m_LIS[newidx1].push_back(set0);
    m_process_S(newidx1, m_LIS[newidx1].size() - 1, sig_counter, read);
  }

  // Process the 2nd subset
  if (sig_counter == 0)
    read = false;
  const auto& set1 = subsets[1];
  assert(set1.get_length() != 0);
  if (set1.get_length() == 1) {
    m_LIP_mask.wtrue(set1.get_start());
    m_process_P(set1.get_start(), sig_counter, read);
  }
  else {
    const auto newidx1 = set1.get_level();
    m_LIS[newidx1].push_back(set1);
    m_process_S(newidx1, m_LIS[newidx1].size() - 1, sig_counter, read);
  }
}

template class sperr::SPECK1D_INT_DEC<uint64_t>;
template class sperr::SPECK1D_INT_DEC<uint32_t>;
template class sperr::SPECK1D_INT_DEC<uint16_t>;
template class sperr::SPECK1D_INT_DEC<uint8_t>;
