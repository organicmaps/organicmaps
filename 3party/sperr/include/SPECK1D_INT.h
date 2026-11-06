#ifndef SPECK1D_INT_H
#define SPECK1D_INT_H

#include "SPECK_INT.h"

#include <cstring>  // std::memcpy()

namespace sperr {

class Set1D {
  // In an effort to reduce the object size of Set1D, I choose to store only the first 7 bytes of
  //    both the `start` and `length` information of a Set1D. Using another 2 bytes to store the
  //    `part_level` info, this object fits in 16 bytes nicely.
  //
 private:
  std::array<uint8_t, 16> m_16 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

 public:
  void set_start(uint64_t val) { std::memcpy(m_16.data(), &val, 7); };
  void set_length(uint64_t val) { std::memcpy(m_16.data() + 7, &val, 7); };
  void set_level(uint16_t val) { std::memcpy(m_16.data() + 14, &val, 2); };
  auto get_start() const -> uint64_t
  {
    auto val = uint64_t{0};
    std::memcpy(&val, m_16.data(), 7);
    return val;
  }
  auto get_length() const -> uint64_t
  {
    auto val = uint64_t{0};
    std::memcpy(&val, m_16.data() + 7, 7);
    return val;
  }
  auto get_level() const -> uint16_t
  {
    auto val = uint16_t{0};
    std::memcpy(&val, m_16.data() + 14, 2);
    return val;
  }
};

//
// Main SPECK1D_INT class; intended to be the base class of both encoder and decoder.
//
template <typename T>
class SPECK1D_INT : public SPECK_INT<T> {
 protected:
  //
  // Bring members from the base class to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_dims;
  using SPECK_INT<T>::m_coeff_buf;

  // The 1D case is different from 3D and 2D cases in that it implements additional logic that
  //    infers the significance of subsets by where the significant point is. With this
  //    consideration, functions such as m_process_S() and m_process_P() have different signatures
  //    during decoding/encoding, so they're implemented in their respective subclasses.
  //
  void m_clean_LIS() final;
  void m_initialize_lists() final;

  auto m_partition_set(Set1D) const -> std::array<Set1D, 2>;

  //
  // SPECK1D_INT specific data members
  //
  std::vector<std::vector<Set1D>> m_LIS;
};

}  // namespace sperr

#endif
