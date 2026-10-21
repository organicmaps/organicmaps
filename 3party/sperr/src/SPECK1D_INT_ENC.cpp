#include "SPECK1D_INT_ENC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

#if __cplusplus >= 202002L
#include <bit>
#endif

template <typename T>
void sperr::SPECK1D_INT_ENC<T>::m_sorting_pass()
{
  // Since we have a separate representation of LIP, let's process that list first!
  //
  const auto bits_x64 = m_LIP_mask.size() - m_LIP_mask.size() % 64;

  for (size_t i = 0; i < bits_x64; i += 64) {
    auto value = m_LIP_mask.rlong(i);

#if __cplusplus >= 202002L
    while (value) {
      size_t j = std::countr_zero(value);
      m_process_P(i + j, SigType::Dunno, j, true);
      value &= value - 1;
    }
#else
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          size_t dummy = 0;
          m_process_P(i + j, SigType::Dunno, dummy, true);
        }
      }
    }
#endif
  }
  for (auto i = bits_x64; i < m_LIP_mask.size(); i++) {
    if (m_LIP_mask.rbit(i)) {
      size_t dummy = 0;
      m_process_P(i, SigType::Dunno, dummy, true);
    }
  }

  // Then we process regular sets in LIS.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    // From the end of m_LIS to its front
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      size_t dummy = 0;
      m_process_S(idx1, idx2, SigType::Dunno, dummy, true);
    }
  }
}

template <typename T>
void sperr::SPECK1D_INT_ENC<T>::m_process_S(size_t idx1,
                                            size_t idx2,
                                            SigType sig,
                                            size_t& counter,
                                            bool output)
{
  auto& set = m_LIS[idx1][idx2];

  // Strategy to decide the significance of this set;
  // 1) If sig == dunno, then find the significance of this set. We do it in a
  //    way that at least one of its 2 subsets' significance become known as well.
  // 2) If sig is significant, then we directly proceed to `m_code_s()`, with its
  //    subsets' significance is unknown.
  // 3) if sig is insignificant, then this set is not processed.
  //
  auto subset_sigs = std::array<SigType, 2>{SigType::Dunno, SigType::Dunno};

  if (sig == SigType::Dunno) {
    auto set_sig = m_decide_significance(set);
    sig = set_sig ? SigType::Sig : SigType::Insig;
    if (set_sig) {
      if (*set_sig < set.get_length() - set.get_length() / 2)
        subset_sigs = {SigType::Sig, SigType::Dunno};
      else
        subset_sigs = {SigType::Insig, SigType::Sig};
    }
  }

  if (output)
    m_bit_buffer.wbit(sig == SigType::Sig);

  if (sig == SigType::Sig) {
    counter++;  // Let's increment the counter first!
    m_code_S(idx1, idx2, subset_sigs);
    set.set_length(0);  // this current set is gonna be discarded.
  }
}

template <typename T>
void sperr::SPECK1D_INT_ENC<T>::m_process_P(size_t idx, SigType sig, size_t& counter, bool output)
{
  // Decide the significance of this pixel
  bool is_sig = false;
  if (sig == SigType::Dunno)
    is_sig = (m_coeff_buf[idx] >= m_threshold);
  else
    is_sig = (sig == SigType::Sig);

  if (output)
    m_bit_buffer.wbit(is_sig);

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_bit_buffer.wbit(m_sign_array.rbit(idx));

    assert(m_coeff_buf[idx] >= m_threshold);
    m_coeff_buf[idx] -= m_threshold;
    m_LSP_new.push_back(idx);
    m_LIP_mask.wfalse(idx);
  }
}

template <typename T>
void sperr::SPECK1D_INT_ENC<T>::m_code_S(size_t idx1,
                                         size_t idx2,
                                         std::array<SigType, 2> subset_sigs)
{
  auto subsets = m_partition_set(m_LIS[idx1][idx2]);
  auto sig_counter = size_t{0};
  auto output = bool{true};

  // Process the 1st subset
  const auto& set0 = subsets[0];
  assert(set0.get_length() != 0);
  if (set0.get_length() == 1) {
    m_LIP_mask.wtrue(set0.get_start());
    m_process_P(set0.get_start(), subset_sigs[0], sig_counter, output);
  }
  else {
    const auto newidx1 = set0.get_level();
    m_LIS[newidx1].emplace_back(set0);
    const auto newidx2 = m_LIS[newidx1].size() - 1;
    m_process_S(newidx1, newidx2, subset_sigs[0], sig_counter, output);
  }

  // Process the 2nd subset
  if (sig_counter == 0) {
    output = false;
    subset_sigs[1] = SigType::Sig;
  }
  const auto& set1 = subsets[1];
  assert(set1.get_length() != 0);
  if (set1.get_length() == 1) {
    m_LIP_mask.wtrue(set1.get_start());
    m_process_P(set1.get_start(), subset_sigs[1], sig_counter, output);
  }
  else {
    const auto newidx1 = set1.get_level();
    m_LIS[newidx1].emplace_back(set1);
    const auto newidx2 = m_LIS[newidx1].size() - 1;
    m_process_S(newidx1, newidx2, subset_sigs[1], sig_counter, output);
  }
}

template <typename T>
auto sperr::SPECK1D_INT_ENC<T>::m_decide_significance(const Set1D& set) const
    -> std::optional<size_t>
{
  assert(set.get_length() != 0);

  const auto gtr = [thld = m_threshold](auto v) { return v >= thld; };
  auto first = m_coeff_buf.cbegin() + set.get_start();
  auto last = first + set.get_length();
  auto found = std::find_if(first, last, gtr);
  if (found != last)
    return static_cast<size_t>(std::distance(first, found));
  else
    return {};
}

template class sperr::SPECK1D_INT_ENC<uint64_t>;
template class sperr::SPECK1D_INT_ENC<uint32_t>;
template class sperr::SPECK1D_INT_ENC<uint16_t>;
template class sperr::SPECK1D_INT_ENC<uint8_t>;
