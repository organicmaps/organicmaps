//
// Four member functions are heavily based on QccPack:
//    http://qccpack.sourceforge.net/index.shtml
//  - void QccWAVCDF97AnalysisSymmetricEvenEven(double* signal, size_t signal_length);
//  - void QccWAVCDF97AnalysisSymmetricOddEven(double* signal, size_t signal_length);
//  - void QccWAVCDF97SynthesisSymmetricEvenEven(double* signal, size_t signal_length);
//  - void QccWAVCDF97SynthesisSymmetricOddEven(double* signal, size_t signal_length);
//

#ifndef CDF97_H
#define CDF97_H

#include "sperr_helper.h"

#include <cmath>

namespace sperr {

class CDF97 {
 public:
  //
  // Destructor
  //
  ~CDF97();

  //
  // Input
  //
  // Note that copy_data() and take_data() effectively resets internal states of this class.
  template <typename T>
  auto copy_data(const T* buf, size_t len, dims_type dims) -> RTNType;
  auto take_data(vecd_type&& buf, dims_type dims) -> RTNType;

  //
  // Output
  //
  auto view_data() const -> const vecd_type&;
  auto release_data() -> vecd_type&&;
  auto get_dims() const -> std::array<size_t, 3>;  // In 2D case, the 3rd value equals 1.

  //
  // Action items
  //
  void dwt1d();
  void dwt2d();
  void dwt3d();
  void idwt2d();
  void idwt1d();
  void idwt3d();

  //
  // Multi-resolution reconstruction
  //

  // If multi-resolution is supported (determined by `sperr::available_resolutions()`), then
  //    it returns all the coarsened volumes, which are placed in the same order of resolutions
  //    returned by `sperr::available_resolutions()`. The native resolution reconstruction should
  //    still be retrieved by the `view_data()` or `release_data()` functions.
  //    If multi-resolution is not supported, then it simply returns an empty vector, with the
  //    decompression still performed, and the native resolution reconstruction ready.
  [[nodiscard]] auto idwt2d_multi_res() -> std::vector<vecd_type>;
  void idwt3d_multi_res(std::vector<vecd_type>&);

 private:
  //
  // Private methods helping DWT.
  //

  // Multiple levels of 1D DWT/IDWT on a given array of length array_len.
  void m_dwt1d(double* array, size_t array_len, size_t num_of_xforms);
  void m_idwt1d(double* array, size_t array_len, size_t num_of_xforms);

  // Multiple levels of 2D DWT/IDWT on a given plane by repeatedly invoking
  // m_dwt2d_one_level(). The plane has a dimension (len_xy[0], len_xy[1]).
  void m_dwt2d(double* plane, std::array<size_t, 2> len_xy, size_t num_of_xforms);
  void m_idwt2d(double* plane, std::array<size_t, 2> len_xy, size_t num_of_xforms);

  // Perform one level of interleaved 3D dwt/idwt on a given volume (m_dims),
  // specifically on its top left (len_xyz) subset.
  void m_dwt3d_one_level(std::array<size_t, 3> len_xyz);
  void m_idwt3d_one_level(std::array<size_t, 3> len_xyz);

  // Perform one level of 2D dwt/idwt on a given plane (m_dims),
  // specifically on its top left (len_xy) subset.
  void m_dwt2d_one_level(double* plane, std::array<size_t, 2> len_xy);
  void m_idwt2d_one_level(double* plane, std::array<size_t, 2> len_xy);

  // Separate even and odd indexed elements to be at the front and back of the dest array.
  // Interleave low and high pass elements to be at even and odd positions of the dest array.
  // Note: sufficient memory space should be allocated by the caller.
  void m_gather(const double* begin, size_t len, double* dest) const;
  void m_scatter(const double* begin, size_t len, double* dest) const;

  // Two flavors of 3D transforms.
  // They should be invoked by the `dwt3d()` and `idwt3d()` public methods, not users, though.
  void m_dwt3d_wavelet_packet();
  void m_idwt3d_wavelet_packet();
  void m_dwt3d_dyadic(size_t num_xforms);
  void m_idwt3d_dyadic(size_t num_xforms);

  // Extract a sub-slice/sub-volume starting with the same origin of the full slice/volume.
  // It is UB if `subdims` exceeds the full dimension (`m_dims`).
  // It is UB if `dst` does not point to a big enough space.
  auto m_sub_slice(std::array<size_t, 2> subdims) const -> vecd_type;
  void m_sub_volume(dims_type subdims, double* dst) const;

  //
  // Methods from QccPack with slight changes to combine the even and odd length cases.
  //
  void QccWAVCDF97AnalysisSymmetric(double* signal, size_t signal_length);
  void QccWAVCDF97SynthesisSymmetric(double* signal, size_t signal_length);

  //
  // Private data members
  //
  vecd_type m_data_buf;          // Holds the entire input data.
  dims_type m_dims = {0, 0, 0};  // Dimension of the data volume

  // Temporary buffers that are big enough for any 1D column or any 2D slice.
  vecd_type m_slice_buf;
  double* m_aligned_buf = nullptr;
  size_t m_aligned_buf_bytes = 0;  // num. of bytes

  //
  // Note on the coefficients and constants:
  // The ones from QccPack are slightly different from what's described in the
  // lifting scheme paper: Pg19 of "FACTORING WAVELET TRANSFORMS INTO LIFTING STEPS,"
  // DAUBECHIES and SWELDEN.  (https://9p.io/who/wim/papers/factor/factor.pdf)
  // JasPer, OpenJPEG, and FFMpeg use coefficients closer to the paper.
  // The filter bank coefficients (h[] array) are from "Biorthogonal Bases of
  // Compactly Supported Wavelets," by Cohen et al., Page 551.
  // (https://services.math.duke.edu/~ingrid/publications/CPAM_1992_p485.pdf)
  //

  // Paper coefficients
  const std::array<double, 5> h = {0.602949018236, 0.266864118443, -0.078223266529, -0.016864118443,
                                   0.026748757411};
  const double r0 = h[0] - 2.0 * h[4] * h[1] / h[3];
  const double r1 = h[2] - h[4] - h[4] * h[1] / h[3];
  const double s0 = h[1] - h[3] - h[3] * r0 / r1;
  const double t0 = h[0] - 2.0 * (h[2] - h[4]);
  const double ALPHA = h[4] / h[3];
  const double BETA = h[3] / r1;
  const double GAMMA = r1 / s0;
  const double DELTA = s0 / t0;
  const double EPSILON = std::sqrt(2.0) * t0;
  const double INV_EPSILON = 1.0 / EPSILON;

  // QccPack coefficients
  //
  // const double ALPHA   = -1.58615986717275;
  // const double BETA    = -0.05297864003258;
  // const double GAMMA   =  0.88293362717904;
  // const double DELTA   =  0.44350482244527;
  // const double EPSILON =  1.14960430535816;
  //
};

}  // namespace sperr

#endif
