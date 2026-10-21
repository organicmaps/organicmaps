#ifndef SPECK3D_INT_DEC_H
#define SPECK3D_INT_DEC_H

#include "SPECK3D_INT.h"

namespace sperr {

//
// Main SPECK3D_INT_DEC class
//
template <typename T>
class SPECK3D_INT_DEC final : public SPECK3D_INT<T> {
 private:
  //
  // Bring members from parent classes to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_bit_buffer;
  using SPECK_INT<T>::m_sign_array;
  using SPECK3D_INT<T>::m_LIS;
  using SPECK3D_INT<T>::m_code_S;

  void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool read) final;
  void m_process_P(size_t idx, size_t no_use, size_t& counter, bool read) final;
  void m_process_P_lite(size_t idx) final;
};

}  // namespace sperr

#endif
