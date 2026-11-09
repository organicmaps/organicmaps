#ifndef SPECK2D_INT_H
#define SPECK2D_INT_H

#include "SPECK_INT.h"

namespace sperr {

class Set2D {
 public:
  uint32_t start_x = 0;
  uint32_t start_y = 0;
  uint32_t length_x = 0;
  uint32_t length_y = 0;
  uint16_t part_level = 0;

 public:
  auto is_pixel() const -> bool { return (size_t{length_x} * length_y == 1); };
  auto is_empty() const -> bool { return (size_t{length_x} * length_y == 0); };
  void make_empty() { length_x = 0; };
};

//
// Main SPECK2D_INT class; intended to be the base class of both encoder and decoder.
//
template <typename T>
class SPECK2D_INT : public SPECK_INT<T> {
 protected:
  //
  // Bring members from the base class to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_dims;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_coeff_buf;
  using SPECK_INT<T>::m_bit_buffer;

  void m_sorting_pass() final;
  void m_clean_LIS() final;
  void m_initialize_lists() final;

  void m_code_S(size_t idx1, size_t idx2);
  void m_code_I();

  virtual void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool need_decide) = 0;
  virtual void m_process_P(size_t idx, size_t& counter, bool need_decide) = 0;
  virtual void m_process_I(bool need_decide) = 0;
  virtual void m_additional_initialization() {};

  auto m_partition_S(Set2D) const -> std::array<Set2D, 4>;
  auto m_partition_I() -> std::array<Set2D, 3>;

  //
  // SPECK2D_INT specific data members
  //
  Set2D m_I;
  std::vector<std::vector<Set2D>> m_LIS;
};

}  // namespace sperr

#endif
