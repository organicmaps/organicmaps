#ifndef SPECK1D_INT_DEC_H
#define SPECK1D_INT_DEC_H

#include "SPECK1D_INT.h"

namespace sperr {

//
// Main SPECK1D_INT_DEC class
//
template <typename T>
class SPECK1D_INT_DEC final : public SPECK1D_INT<T> {
 private:
  //
  // Bring members from parent classes to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_bit_buffer;
  using SPECK_INT<T>::m_sign_array;
  using SPECK1D_INT<T>::m_LIS;
  using SPECK1D_INT<T>::m_partition_set;

  void m_sorting_pass() final;

  void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool read);
  void m_process_P(size_t idx, size_t& counter, bool);
  void m_code_S(size_t idx1, size_t idx2);
};

}  // namespace sperr

#endif
