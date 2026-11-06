#ifndef SPECK2D_INT_DEC_H
#define SPECK2D_INT_DEC_H

#include "SPECK2D_INT.h"

namespace sperr {

//
// Main SPECK2D_INT_DEC class
//
template <typename T>
class SPECK2D_INT_DEC final : public SPECK2D_INT<T> {
 private:
  //
  // Bring members from parent classes to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_dims;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_bit_buffer;
  using SPECK_INT<T>::m_sign_array;
  using SPECK2D_INT<T>::m_LIS;
  using SPECK2D_INT<T>::m_I;
  using SPECK2D_INT<T>::m_code_S;
  using SPECK2D_INT<T>::m_code_I;

  void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool need_decide) final;
  void m_process_P(size_t idx, size_t& counter, bool need_decide) final;
  void m_process_I(bool need_decide) final;
};

}  // namespace sperr

#endif
