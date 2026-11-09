#ifndef SPECK2D_INT_ENC_H
#define SPECK2D_INT_ENC_H

#include "SPECK2D_INT.h"

namespace sperr {

//
// Main SPECK2D_INT_ENC class
//
template <typename T>
class SPECK2D_INT_ENC final : public SPECK2D_INT<T> {
 private:
  //
  // Bring members from parent classes to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_dims;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_threshold;
  using SPECK_INT<T>::m_coeff_buf;
  using SPECK_INT<T>::m_bit_buffer;
  using SPECK_INT<T>::m_sign_array;
  using SPECK2D_INT<T>::m_LIS;
  using SPECK2D_INT<T>::m_I;
  using SPECK2D_INT<T>::m_code_S;
  using SPECK2D_INT<T>::m_code_I;

  void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool need_decide) final;
  void m_process_P(size_t idx, size_t& counter, bool need_decide) final;
  void m_process_I(bool need_decide) final;
  void m_additional_initialization() final;
  void m_bitplane_init() final;
  void m_refinement_extra() final;

  auto m_decide_S_significance(const Set2D&) const -> bool;
  auto m_decide_I_significance() const -> bool;

  // `m_msb_buf` stores the MSB bit position of each coefficient, in the same order as
  // m_coeff_buf. Significance tests compare entries against `m_msb_threshold`.
  std::vector<int8_t> m_msb_buf;
  int8_t m_msb_threshold = -1;
};

}  // namespace sperr

#endif
