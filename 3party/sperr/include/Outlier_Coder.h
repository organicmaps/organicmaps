#ifndef OUTLIER_CODER_H
#define OUTLIER_CODER_H

#include "SPECK1D_INT_DEC.h"
#include "SPECK1D_INT_ENC.h"
#include "sperr_helper.h"

#include <variant>

namespace sperr {

class Outlier {
 public:
  size_t pos = 0;
  double err = 0.0;

  Outlier() = default;
  Outlier(size_t, double);
};

class Outlier_Coder {
 public:
  //
  // Input
  //
  void add_outlier(Outlier);
  void use_outlier_list(std::vector<Outlier>);
  void set_length(size_t);
  void set_tolerance(double);
  auto use_bitstream(const void*, size_t) -> RTNType;

  //
  // Output
  //
  auto view_outlier_list() const -> const std::vector<Outlier>&;
  void append_encoded_bitstream(vec8_type& buf) const;
  auto get_stream_full_len(const void*) const -> size_t;

  //
  // Action items
  //
  auto encode() -> RTNType;
  auto decode() -> RTNType;

 private:
  size_t m_total_len = 0;
  double m_tol = 0.0;
  Bitmask m_sign_array;
  std::vector<Outlier> m_LOS;

  std::variant<SPECK1D_INT_ENC<uint8_t>,
               SPECK1D_INT_ENC<uint16_t>,
               SPECK1D_INT_ENC<uint32_t>,
               SPECK1D_INT_ENC<uint64_t>>
      m_encoder;

  std::variant<SPECK1D_INT_DEC<uint8_t>,
               SPECK1D_INT_DEC<uint16_t>,
               SPECK1D_INT_DEC<uint32_t>,
               SPECK1D_INT_DEC<uint64_t>>
      m_decoder;

  std::variant<std::vector<uint8_t>,
               std::vector<uint16_t>,
               std::vector<uint32_t>,
               std::vector<uint64_t>>
      m_vals_ui;

  void m_instantiate_uvec_coders(UINTType);
  void m_quantize();
  void m_inverse_quantize();
};

}  // namespace sperr

#endif
