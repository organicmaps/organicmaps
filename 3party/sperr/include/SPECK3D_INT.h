#ifndef SPECK3D_INT_H
#define SPECK3D_INT_H

#include "SPECK_INT.h"

#include <tuple>

namespace sperr {

class Set3D {
 public:
  uint64_t morton_idx = 0;
  uint16_t start_x = 0;
  uint16_t start_y = 0;
  uint16_t start_z = 0;
  uint16_t length_x = 0;
  uint16_t length_y = 0;
  uint16_t length_z = 0;

  void make_empty() { length_x = 0; }
  auto num_elem() const -> size_t { return (size_t{length_x} * length_y * length_z); }
};

//
// Main SPECK3D_INT class; intended to be the base class of both encoder and decoder.
//
template <typename T>
class SPECK3D_INT : public SPECK_INT<T> {
 protected:
  //
  // Bring members from the base class to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_dims;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_coeff_buf;
  using SPECK_INT<T>::m_bit_buffer;

  void m_initialize_lists() final;
  void m_sorting_pass() final;
  void m_clean_LIS() final;

  virtual void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool) = 0;
  virtual void m_process_P(size_t i, size_t m, size_t& c, bool) = 0;  // Called by `m_code_S()`.
  virtual void m_process_P_lite(size_t idx) = 0;  // Called by `m_sorting_pass()` directly.
  virtual void m_additional_initialization() {};  // empty by default

  void m_code_S(size_t idx1, size_t idx2);
  auto m_partition_S_XYZ(Set3D, uint16_t) const -> std::tuple<std::array<Set3D, 8>, uint16_t>;
  auto m_partition_S_XY(Set3D, uint16_t) const -> std::tuple<std::array<Set3D, 4>, uint16_t>;
  auto m_partition_S_Z(Set3D, uint16_t) const -> std::tuple<std::array<Set3D, 2>, uint16_t>;

  //
  // SPECK3D_INT specific data members
  //
  std::vector<std::vector<Set3D>> m_LIS;
};

}  // namespace sperr

#endif
