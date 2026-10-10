#ifndef SPECK_FLT_H
#define SPECK_FLT_H

//
// This class serves as the base class of 1D, 2D, and 3D SPECK algorithm on floats.
//

#include "CDF97.h"
#include "Conditioner.h"
#include "Outlier_Coder.h"
#include "SPECK_INT.h"

#include <variant>

namespace sperr {

class SPECK_FLT {
 public:
  //
  // Virtual Destructor
  //
  virtual ~SPECK_FLT() = default;

  //
  // Input
  //
  // Accept incoming data: copy from a raw memory block.
  // Note: `len` is the number of values.
  template <typename T>
  void copy_data(const T* p, size_t len);

  // Accept incoming data: take ownership of a memory block
  void take_data(std::vector<double>&&);

  // Use an encoded bitstream
  // Note: `len` is the number of bytes.
  virtual auto use_bitstream(const void* p, size_t len) -> RTNType;

  //
  // Output
  //
  void append_encoded_bitstream(vec8_type& buf) const;
  auto view_decoded_data() const -> const vecd_type&;
  auto view_hierarchy() const -> const std::vector<vecd_type>&;
  auto release_decoded_data() -> vecd_type&&;
  auto release_hierarchy() -> std::vector<vecd_type>&&;

  //
  // General configuration and info.
  //
  void set_psnr(double psnr);
  void set_tolerance(double tol);
  void set_bitrate(double bpp);
  void set_dims(dims_type);
  auto integer_len() const -> size_t;

#ifdef EXPERIMENTING
  void set_direct_q(double q);
#endif

  //
  // Actions
  //
  auto compress() -> RTNType;
  auto decompress(bool multi_res = false) -> RTNType;

 protected:
  UINTType m_uint_flag = UINTType::UINT64;
  bool m_has_outlier = false;           // encoding (PWE mode) and decoding
  CompMode m_mode = CompMode::Unknown;  // encoding only
  double m_q = 0.0;                     // encoding and decoding
  double m_quality = 0.0;               // encoding only, represent either PSNR, PWE, or BPP.
  vecd_type m_vals_orig;                // encoding only (PWE mode)
  dims_type m_dims = {0, 0, 0};
  vecd_type m_vals_d;
  condi_type m_condi_bitstream;
  Bitmask m_sign_array;
  std::vector<vecd_type> m_hierarchy;  // multi-resolution decoding

  CDF97 m_cdf;
  Conditioner m_conditioner;
  Outlier_Coder m_out_coder;

  std::variant<std::vector<uint8_t>,
               std::vector<uint16_t>,
               std::vector<uint32_t>,
               std::vector<uint64_t>>
      m_vals_ui;

  std::variant<std::unique_ptr<SPECK_INT<uint8_t>>,
               std::unique_ptr<SPECK_INT<uint16_t>>,
               std::unique_ptr<SPECK_INT<uint32_t>>,
               std::unique_ptr<SPECK_INT<uint64_t>>>
      m_encoder, m_decoder;

  // Instantiate `m_vals_ui` based on the chosen integer length.
  void m_instantiate_int_vec();

  // Derived classes instantiate the correct `m_encoder` and `m_decoder` depending on
  // 3D/2D/1D classes, and on the integer length in use.
  virtual void m_instantiate_encoder() = 0;
  virtual void m_instantiate_decoder() = 0;

  // Both wavelet transforms operate on `m_vals_d`.
  virtual void m_wavelet_xform() = 0;
  virtual void m_inverse_wavelet_xform(bool multi_res) = 0;

  // This base class provides two midtread quantization implementations.
  //    Quantization reads from `m_vals_d`, and writes to `m_vals_ui` and `m_sign_array`.
  //    Inverse quantization reads from `m_vals_ui` and `m_sign_array`, and writes to `m_vals_d`.
  auto m_midtread_quantize() -> RTNType;
  void m_midtread_inv_quantize();

  // Estimate MSE assuming midtread quantization strategy.
  auto m_estimate_mse_midtread(double q) const -> double;

  // The meaning of inputs `param` and `high_prec` differ depending on the compression mode:
  //    - PWE:  no input is used; they can be anything;
  //    - PSNR: `param` must be the data range of the original input; `high_prec` is not used;
  //    - Rate: `param` must be the biggest magnitude of transformed wavelet coefficients;
  //            `high_prec` should be false at first, and true if not enough bits are produced.
  auto m_estimate_q(double param, bool high_prec) const -> double;
};

}  // namespace sperr

#endif
