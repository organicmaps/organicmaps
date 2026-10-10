#include "CDF97.h"

#include <algorithm>
#include <cassert>
#include <numeric>  // std::accumulate()
#include <type_traits>

#ifdef __AVX2__
#include <immintrin.h>
#endif

// Destructor
sperr::CDF97::~CDF97()
{
  if (m_aligned_buf)
    sperr::aligned_free(m_aligned_buf);
}

template <typename T>
auto sperr::CDF97::copy_data(const T* data, size_t len, dims_type dims) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");
  if (len != dims[0] * dims[1] * dims[2])
    return RTNType::WrongLength;

  m_data_buf.resize(len);
  std::copy(data, data + len, m_data_buf.begin());

  m_dims = dims;

  auto max_col = std::max(std::max(dims[0], dims[1]), dims[2]);
  if (max_col * sizeof(double) > m_aligned_buf_bytes) {
    if (m_aligned_buf)
      sperr::aligned_free(m_aligned_buf);
    size_t alignment = 32;  // 256 bits
    size_t alloc_chunks = (max_col * 8 + 31) / alignment;
    m_aligned_buf_bytes = alignment * alloc_chunks;
    m_aligned_buf = static_cast<double*>(sperr::aligned_malloc(alignment, m_aligned_buf_bytes));
  }

  auto max_slice = std::max(std::max(dims[0] * dims[1], dims[0] * dims[2]), dims[1] * dims[2]);
  if (max_slice > m_slice_buf.size())
    m_slice_buf.resize(max_slice);

  return RTNType::Good;
}
template auto sperr::CDF97::copy_data(const float*, size_t, dims_type) -> RTNType;
template auto sperr::CDF97::copy_data(const double*, size_t, dims_type) -> RTNType;

auto sperr::CDF97::take_data(vecd_type&& buf, dims_type dims) -> RTNType
{
  if (buf.size() != dims[0] * dims[1] * dims[2])
    return RTNType::WrongLength;

  m_data_buf = std::move(buf);
  m_dims = dims;

  auto max_col = std::max(std::max(dims[0], dims[1]), dims[2]);
  if (max_col * sizeof(double) > m_aligned_buf_bytes) {
    if (m_aligned_buf)
      sperr::aligned_free(m_aligned_buf);
    size_t alignment = 32;  // 256 bits
    size_t alloc_chunks = (max_col * 8 + 31) / alignment;
    m_aligned_buf_bytes = alignment * alloc_chunks;
    m_aligned_buf = static_cast<double*>(sperr::aligned_malloc(alignment, m_aligned_buf_bytes));
  }

  auto max_slice = std::max(std::max(dims[0] * dims[1], dims[0] * dims[2]), dims[1] * dims[2]);
  if (max_slice > m_slice_buf.size())
    m_slice_buf.resize(max_slice);

  return RTNType::Good;
}

auto sperr::CDF97::view_data() const -> const vecd_type&
{
  return m_data_buf;
}

auto sperr::CDF97::release_data() -> vecd_type&&
{
  return std::move(m_data_buf);
}

auto sperr::CDF97::get_dims() const -> std::array<size_t, 3>
{
  return m_dims;
}

void sperr::CDF97::dwt1d()
{
  auto num_xforms = sperr::num_of_xforms(m_dims[0]);
  m_dwt1d(m_data_buf.data(), m_data_buf.size(), num_xforms);
}

void sperr::CDF97::idwt1d()
{
  auto num_xforms = sperr::num_of_xforms(m_dims[0]);
  m_idwt1d(m_data_buf.data(), m_data_buf.size(), num_xforms);
}

void sperr::CDF97::dwt2d()
{
  auto xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  m_dwt2d(m_data_buf.data(), {m_dims[0], m_dims[1]}, xy);
}

void sperr::CDF97::idwt2d()
{
  auto xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  m_idwt2d(m_data_buf.data(), {m_dims[0], m_dims[1]}, xy);
}

auto sperr::CDF97::idwt2d_multi_res() -> std::vector<vecd_type>
{
  const auto xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  auto ret = std::vector<vecd_type>();

  if (xy > 0) {
    ret.reserve(xy);
    for (size_t lev = xy; lev > 0; lev--) {
      auto [x, xd] = sperr::calc_approx_detail_len(m_dims[0], lev);
      auto [y, yd] = sperr::calc_approx_detail_len(m_dims[1], lev);
      ret.emplace_back(m_sub_slice({x, y}));
      m_idwt2d_one_level(m_data_buf.data(), {x + xd, y + yd});
    }
  }

  return ret;
}

void sperr::CDF97::dwt3d()
{
  auto dyadic = sperr::can_use_dyadic(m_dims);
  if (dyadic)
    m_dwt3d_dyadic(*dyadic);
  else
    m_dwt3d_wavelet_packet();
}

void sperr::CDF97::idwt3d()
{
  auto dyadic = sperr::can_use_dyadic(m_dims);
  if (dyadic)
    m_idwt3d_dyadic(*dyadic);
  else
    m_idwt3d_wavelet_packet();
}

void sperr::CDF97::idwt3d_multi_res(std::vector<vecd_type>& h)
{
  auto dyadic = sperr::can_use_dyadic(m_dims);

  if (dyadic) {
    h.resize(*dyadic);
    for (size_t lev = *dyadic; lev > 0; lev--) {
      auto [x, xd] = sperr::calc_approx_detail_len(m_dims[0], lev);
      auto [y, yd] = sperr::calc_approx_detail_len(m_dims[1], lev);
      auto [z, zd] = sperr::calc_approx_detail_len(m_dims[2], lev);
      auto& buf = h[*dyadic - lev];
      buf.resize(x * y * z);
      m_sub_volume({x, y, z}, buf.data());
      m_idwt3d_one_level({x + xd, y + yd, z + zd});
    }
  }
  else
    m_idwt3d_wavelet_packet();
}

void sperr::CDF97::m_dwt3d_wavelet_packet()
{
  /*
   *             Z
   *            /
   *           /
   *          /________
   *         /       /|
   *        /       / |
   *     0 |-------/-------> X
   *       |       |  |
   *       |       |  /
   *       |       | /
   *       |_______|/
   *       |
   *       |
   *       Y
   */

  const size_t plane_size_xy = m_dims[0] * m_dims[1];

  // First transform along the Z dimension
  //
  const auto num_xforms_z = sperr::num_of_xforms(m_dims[2]);

  for (size_t y = 0; y < m_dims[1]; y++) {
    const auto y_offset = y * m_dims[0];

    // Re-arrange values of one XZ slice so that they form many z_columns
    for (size_t z = 0; z < m_dims[2]; z++) {
      const auto cube_start_idx = z * plane_size_xy + y_offset;
      for (size_t x = 0; x < m_dims[0]; x++)
        m_slice_buf[z + x * m_dims[2]] = m_data_buf[cube_start_idx + x];
    }

    // DWT1D on every z_column
    for (size_t x = 0; x < m_dims[0]; x++)
      m_dwt1d(m_slice_buf.data() + x * m_dims[2], m_dims[2], num_xforms_z);

    // Put back values of the z_columns to the cube
    for (size_t z = 0; z < m_dims[2]; z++) {
      const auto cube_start_idx = z * plane_size_xy + y_offset;
      for (size_t x = 0; x < m_dims[0]; x++)
        m_data_buf[cube_start_idx + x] = m_slice_buf[z + x * m_dims[2]];
    }
  }

  // Second transform each plane
  //
  const auto num_xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));

  for (size_t z = 0; z < m_dims[2]; z++) {
    const size_t offset = plane_size_xy * z;
    m_dwt2d(m_data_buf.data() + offset, {m_dims[0], m_dims[1]}, num_xforms_xy);
  }
}

void sperr::CDF97::m_idwt3d_wavelet_packet()
{
  const size_t plane_size_xy = m_dims[0] * m_dims[1];

  // First, inverse transform each plane
  //
  auto num_xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  for (size_t i = 0; i < m_dims[2]; i++) {
    const size_t offset = plane_size_xy * i;
    m_idwt2d(m_data_buf.data() + offset, {m_dims[0], m_dims[1]}, num_xforms_xy);
  }

  /*
   * Second, inverse transform along the Z dimension
   *
   *             Z
   *            /
   *           /
   *          /________
   *         /       /|
   *        /       / |
   *     0 |-------/-------> X
   *       |       |  |
   *       |       |  /
   *       |       | /
   *       |_______|/
   *       |
   *       |
   *       Y
   */

  // Process one XZ slice at a time
  //
  const auto num_xforms_z = sperr::num_of_xforms(m_dims[2]);
  for (size_t y = 0; y < m_dims[1]; y++) {
    const auto y_offset = y * m_dims[0];

    // Re-arrange values on one slice so that they form many z_columns
    for (size_t z = 0; z < m_dims[2]; z++) {
      const auto cube_start_idx = z * plane_size_xy + y_offset;
      for (size_t x = 0; x < m_dims[0]; x++)
        m_slice_buf[z + x * m_dims[2]] = m_data_buf[cube_start_idx + x];
    }

    // IDWT1D on every z_column
    for (size_t x = 0; x < m_dims[0]; x++)
      m_idwt1d(m_slice_buf.data() + x * m_dims[2], m_dims[2], num_xforms_z);

    // Put back values from the z_columns to the cube
    for (size_t z = 0; z < m_dims[2]; z++) {
      const auto cube_start_idx = z * plane_size_xy + y_offset;
      for (size_t x = 0; x < m_dims[0]; x++)
        m_data_buf[cube_start_idx + x] = m_slice_buf[z + x * m_dims[2]];
    }
  }
}

void sperr::CDF97::m_dwt3d_dyadic(size_t num_xforms)
{
  for (size_t lev = 0; lev < num_xforms; lev++) {
    auto [x, xd] = sperr::calc_approx_detail_len(m_dims[0], lev);
    auto [y, yd] = sperr::calc_approx_detail_len(m_dims[1], lev);
    auto [z, zd] = sperr::calc_approx_detail_len(m_dims[2], lev);
    m_dwt3d_one_level({x, y, z});
  }
}

void sperr::CDF97::m_idwt3d_dyadic(size_t num_xforms)
{
  for (size_t lev = num_xforms; lev > 0; lev--) {
    auto [x, xd] = sperr::calc_approx_detail_len(m_dims[0], lev - 1);
    auto [y, yd] = sperr::calc_approx_detail_len(m_dims[1], lev - 1);
    auto [z, zd] = sperr::calc_approx_detail_len(m_dims[2], lev - 1);
    m_idwt3d_one_level({x, y, z});
  }
}

//
// Private Methods
//
void sperr::CDF97::m_dwt1d(double* array, size_t array_len, size_t num_of_lev)
{
  for (size_t lev = 0; lev < num_of_lev; lev++) {
    m_gather(array, array_len, m_aligned_buf);
    this->QccWAVCDF97AnalysisSymmetric(m_aligned_buf, array_len);
    std::copy(m_aligned_buf, m_aligned_buf + array_len, array);
    array_len -= array_len / 2;
  }
}

void sperr::CDF97::m_idwt1d(double* array, size_t array_len, size_t num_of_lev)
{
  for (size_t lev = num_of_lev; lev > 0; lev--) {
    auto [x, xd] = sperr::calc_approx_detail_len(array_len, lev - 1);
    this->QccWAVCDF97SynthesisSymmetric(array, x);
    m_scatter(array, x, m_aligned_buf);
    std::copy(m_aligned_buf, m_aligned_buf + x, array);
  }
}

void sperr::CDF97::m_dwt2d(double* plane, std::array<size_t, 2> len_xy, size_t num_of_lev)
{
  for (size_t lev = 0; lev < num_of_lev; lev++) {
    auto [x, xd] = sperr::calc_approx_detail_len(len_xy[0], lev);
    auto [y, yd] = sperr::calc_approx_detail_len(len_xy[1], lev);
    m_dwt2d_one_level(plane, {x, y});
  }
}

void sperr::CDF97::m_idwt2d(double* plane, std::array<size_t, 2> len_xy, size_t num_of_lev)
{
  for (size_t lev = num_of_lev; lev > 0; lev--) {
    auto [x, xd] = sperr::calc_approx_detail_len(len_xy[0], lev - 1);
    auto [y, yd] = sperr::calc_approx_detail_len(len_xy[1], lev - 1);
    m_idwt2d_one_level(plane, {x, y});
  }
}

void sperr::CDF97::m_dwt2d_one_level(double* plane, std::array<size_t, 2> len_xy)
{
  // First, perform DWT along X for every row
  for (size_t i = 0; i < len_xy[1]; i++) {
    auto* pos = plane + i * m_dims[0];
    m_gather(pos, len_xy[0], m_aligned_buf);
    this->QccWAVCDF97AnalysisSymmetric(m_aligned_buf, len_xy[0]);
    std::copy(m_aligned_buf, m_aligned_buf + len_xy[0], pos);
  }

  // Second, perform DWT along Y for every column
  for (size_t x = 0; x < len_xy[0]; x++) {
    for (size_t y = 0; y < len_xy[1]; y++)
      m_slice_buf[y] = plane[y * m_dims[0] + x];
    m_gather(m_slice_buf.data(), len_xy[1], m_aligned_buf);
    this->QccWAVCDF97AnalysisSymmetric(m_aligned_buf, len_xy[1]);
    for (size_t y = 0; y < len_xy[1]; y++)
      plane[y * m_dims[0] + x] = m_aligned_buf[y];
  }
}

void sperr::CDF97::m_idwt2d_one_level(double* plane, std::array<size_t, 2> len_xy)
{
  // First, perform IDWT along Y for every column
  for (size_t x = 0; x < len_xy[0]; x++) {
    for (size_t y = 0; y < len_xy[1]; y++)
      m_slice_buf[y] = plane[y * m_dims[0] + x];
    this->QccWAVCDF97SynthesisSymmetric(m_slice_buf.data(), len_xy[1]);
    m_scatter(m_slice_buf.data(), len_xy[1], m_aligned_buf);
    for (size_t y = 0; y < len_xy[1]; y++)
      plane[y * m_dims[0] + x] = m_aligned_buf[y];
  }

  // Second, perform IDWT along X for every row
  for (size_t i = 0; i < len_xy[1]; i++) {
    auto* pos = plane + i * m_dims[0];
    this->QccWAVCDF97SynthesisSymmetric(pos, len_xy[0]);
    m_scatter(pos, len_xy[0], m_aligned_buf);
    std::copy(m_aligned_buf, m_aligned_buf + len_xy[0], pos);
  }
}

void sperr::CDF97::m_dwt3d_one_level(std::array<size_t, 3> len_xyz)
{
  // First, do one level of transform on all XY planes.
  const auto plane_size_xy = m_dims[0] * m_dims[1];
  const auto col_len = len_xyz[2];
  for (size_t z = 0; z < col_len; z++) {
    const size_t offset = plane_size_xy * z;
    m_dwt2d_one_level(m_data_buf.data() + offset, {len_xyz[0], len_xyz[1]});
  }

  // Second, do one level of transform on all Z columns.  Strategy:
  // 1) extract eight Z columns to buffer space `m_slice_buf`
  // 2) transform these eight columns
  // 3) put the Z columns back to their locations in the volume.
  //
  // Note: the reason to process eight columns at a time is that a cache line
  // is usually 64 bytes, or 8 doubles. That means when you pay the cost to retrieve
  // one value from the Z column, its neighboring 7 values are available for free!

  for (size_t y = 0; y < len_xyz[1]; y++) {
    for (size_t x = 0; x < len_xyz[0]; x += 8) {
      const size_t xy_offset = y * m_dims[0] + x;
      const size_t stride = std::min(size_t{8}, len_xyz[0] - x);

      for (size_t z = 0; z < col_len; z++) {
        for (size_t i = 0; i < stride; i++)
          m_slice_buf[z + i * col_len] = m_data_buf[z * plane_size_xy + xy_offset + i];
      }

      for (size_t i = 0; i < stride; i++) {
        auto* itr = m_slice_buf.data() + i * col_len;
        m_gather(itr, col_len, m_aligned_buf);
        this->QccWAVCDF97AnalysisSymmetric(m_aligned_buf, col_len);
        std::copy(m_aligned_buf, m_aligned_buf + col_len, itr);
      }

      for (size_t z = 0; z < col_len; z++) {
        for (size_t i = 0; i < stride; i++)
          m_data_buf[z * plane_size_xy + xy_offset + i] = m_slice_buf[z + i * col_len];
      }
    }
  }
}

void sperr::CDF97::m_idwt3d_one_level(std::array<size_t, 3> len_xyz)
{
  const auto plane_size_xy = m_dims[0] * m_dims[1];
  const auto col_len = len_xyz[2];

  // First, do one level of inverse transform on all Z columns.  Strategy:
  // 1) extract eight Z columns to buffer space `m_slice_buf`
  // 2) transform these eight columns
  // 3) put the Z columns back to their appropriate locations in the volume.
  //
  // Note: the reason to process eight columns at a time is that a cache line
  // is usually 64 bytes, or 8 doubles. That means when you pay the cost to retrieve
  // one value from the Z column, its neighboring 7 values are available for free!

  for (size_t y = 0; y < len_xyz[1]; y++) {
    for (size_t x = 0; x < len_xyz[0]; x += 8) {
      const size_t xy_offset = y * m_dims[0] + x;
      const size_t stride = std::min(size_t{8}, len_xyz[0] - x);

      for (size_t z = 0; z < col_len; z++) {
        for (size_t i = 0; i < stride; i++)
          m_slice_buf[z + i * col_len] = m_data_buf[z * plane_size_xy + xy_offset + i];
      }

      for (size_t i = 0; i < stride; i++) {
        auto* itr = m_slice_buf.data() + i * col_len;
        this->QccWAVCDF97SynthesisSymmetric(itr, col_len);
        m_scatter(itr, col_len, m_aligned_buf);
        std::copy(m_aligned_buf, m_aligned_buf + col_len, itr);
      }

      for (size_t z = 0; z < col_len; z++) {
        for (size_t i = 0; i < stride; i++)
          m_data_buf[z * plane_size_xy + xy_offset + i] = m_slice_buf[z + i * col_len];
      }
    }
  }

  // Second, do one level of inverse transform on all XY planes.
  for (size_t z = 0; z < len_xyz[2]; z++) {
    const size_t offset = plane_size_xy * z;
    m_idwt2d_one_level(m_data_buf.data() + offset, {len_xyz[0], len_xyz[1]});
  }
}

void sperr::CDF97::m_gather(const double* src, size_t len, double* dst) const
{
#ifdef __AVX2__
  const double* src_end = src + len;
  double* dst_evens = dst;
  double* dst_odds = dst + len - len / 2;

  // Process 8 elements at a time
  for (; src + 8 <= src_end; src += 8) {
    __m256d v0 = _mm256_loadu_pd(src);      // 0, 1, 2, 3
    __m256d v1 = _mm256_loadu_pd(src + 4);  // 4, 5, 6, 7

    __m256d evens = _mm256_unpacklo_pd(v0, v1);  // 0, 4, 2, 6
    __m256d odds = _mm256_unpackhi_pd(v0, v1);   // 1, 5, 3, 7

    __m256d result1 = _mm256_permute4x64_pd(evens, 0b11011000);  // 0, 2, 4, 6
    __m256d result2 = _mm256_permute4x64_pd(odds, 0b11011000);   // 1, 3, 5, 7

    _mm256_store_pd(dst_evens, result1);
    _mm256_storeu_pd(dst_odds, result2);

    dst_evens += 4;
    dst_odds += 4;
  }

  for (; src < src_end - 1; src += 2) {
    *(dst_evens++) = *src;
    *(dst_odds++) = *(src + 1);
  }

  if (src < src_end)
    *dst_evens = *src;
#else
  size_t low_count = len - len / 2, high_count = len / 2;
  for (size_t i = 0; i < low_count; i++) {
    *dst = *(src + i * 2);
    ++dst;
  }
  for (size_t i = 0; i < high_count; i++) {
    *dst = *(src + i * 2 + 1);
    ++dst;
  }
#endif
}

void sperr::CDF97::m_scatter(const double* begin, size_t len, double* dst) const
{
#ifdef __AVX2__
  const double* even_end = begin + len - len / 2;
  const double* odd_beg = even_end;
  const double* dst_end = dst + len;

  // Process 8 elements at a time
  for (; begin + 4 < even_end; begin += 4) {
    __m256d v0 = _mm256_loadu_pd(begin);    // 0, 1, 2, 3
    __m256d v1 = _mm256_loadu_pd(odd_beg);  // 4, 5, 6, 7

    __m256d evens = _mm256_unpacklo_pd(v0, v1);  // 0, 4, 2, 6
    __m256d odds = _mm256_unpackhi_pd(v0, v1);   // 1, 5, 3, 7

    __m256d result1 = _mm256_permute2f128_pd(evens, odds, 0x20);  // 0, 4, 1, 5
    __m256d result2 = _mm256_permute2f128_pd(evens, odds, 0x31);  // 2, 6, 3, 7

    _mm256_store_pd(dst, result1);
    _mm256_store_pd(dst + 4, result2);

    dst += 8;
    odd_beg += 4;
  }

  for (; dst < dst_end - 1; dst += 2) {
    *dst = *(begin++);
    *(dst + 1) = *(odd_beg++);
  }

  if (dst < dst_end)
    *dst = *begin;
#else
  size_t low_count = len - len / 2, high_count = len / 2;
  for (size_t i = 0; i < low_count; i++) {
    *(dst + i * 2) = *begin;
    ++begin;
  }
  for (size_t i = 0; i < high_count; i++) {
    *(dst + i * 2 + 1) = *begin;
    ++begin;
  }
#endif
}

auto sperr::CDF97::m_sub_slice(std::array<size_t, 2> subdims) const -> vecd_type
{
  assert(subdims[0] <= m_dims[0] && subdims[1] <= m_dims[1]);

  auto ret = vecd_type(subdims[0] * subdims[1]);
  auto dst = ret.begin();
  for (size_t y = 0; y < subdims[1]; y++) {
    auto beg = m_data_buf.begin() + y * m_dims[0];
    std::copy(beg, beg + subdims[0], dst);
    dst += subdims[0];
  }

  return ret;
}

void sperr::CDF97::m_sub_volume(dims_type subdims, double* dst) const
{
  assert(subdims[0] <= m_dims[0] && subdims[1] <= m_dims[1] && subdims[2] <= m_dims[2]);

  const auto slice_len = m_dims[0] * m_dims[1];
  for (size_t z = 0; z < subdims[2]; z++) {
    for (size_t y = 0; y < subdims[1]; y++) {
      auto beg = m_data_buf.begin() + z * slice_len + y * m_dims[0];
      std::copy(beg, beg + subdims[0], dst);
      dst += subdims[0];
    }
  }
}

//
// Methods from QccPack
//
void sperr::CDF97::QccWAVCDF97AnalysisSymmetric(double* signal, size_t len)
{
  size_t even_len = len - len / 2;
  size_t odd_len = len / 2;
  double* even = signal;
  double* odd = signal + even_len;

  // Process all the odd elements
  for (size_t i = 0; i < odd_len - 1; i++)
    odd[i] += ALPHA * (even[i] + even[i + 1]);
  odd[odd_len - 1] += ALPHA * (even[odd_len - 1] + even[even_len - 1]);

  // Process all the even elements
  even[0] += 2.0 * BETA * odd[0];
  for (size_t i = 1; i < even_len - 1; i++)
    even[i] += BETA * (odd[i - 1] + odd[i]);
  even[even_len - 1] += BETA * (odd[even_len - 2] + odd[odd_len - 1]);

  // Process all the odd elements
  for (size_t i = 0; i < odd_len - 1; i++)
    odd[i] += GAMMA * (even[i] + even[i + 1]);
  odd[odd_len - 1] += GAMMA * (even[odd_len - 1] + even[even_len - 1]);

  // Process even elements
  even[0] = EPSILON * (even[0] + 2.0 * DELTA * odd[0]);
  for (size_t i = 1; i < even_len - 1; i++)
    even[i] = EPSILON * (even[i] + DELTA * (odd[i - 1] + odd[i]));
  even[even_len - 1] =
      EPSILON * (even[even_len - 1] + DELTA * (odd[even_len - 2] + odd[odd_len - 1]));

  // Process odd elements
  for (size_t i = 0; i < odd_len; i++)
    odd[i] *= -INV_EPSILON;
}

void sperr::CDF97::QccWAVCDF97SynthesisSymmetric(double* signal, size_t len)
{
  size_t even_len = len - len / 2;
  size_t odd_len = len / 2;
  double* even = signal;
  double* odd = signal + even_len;

  // Process odd elements
  for (size_t i = 0; i < odd_len; i++)
    odd[i] *= (-EPSILON);

  // Process even elements
  even[0] = even[0] * INV_EPSILON - 2.0 * DELTA * odd[0];
  for (size_t i = 1; i < even_len - 1; i++)
    even[i] = even[i] * INV_EPSILON - DELTA * (odd[i - 1] + odd[i]);
  even[even_len - 1] =
      even[even_len - 1] * INV_EPSILON - DELTA * (odd[even_len - 2] + odd[odd_len - 1]);

  // Process odd elements
  for (size_t i = 0; i < odd_len - 1; i++)
    odd[i] -= GAMMA * (even[i] + even[i + 1]);
  odd[odd_len - 1] -= GAMMA * (even[odd_len - 1] + even[even_len - 1]);

  // Process even elements
  even[0] -= 2.0 * BETA * odd[0];
  for (size_t i = 1; i < even_len - 1; i++)
    even[i] -= BETA * (odd[i - 1] + odd[i]);
  even[even_len - 1] -= BETA * (odd[even_len - 2] + odd[odd_len - 1]);

  // Process odd elements
  for (size_t i = 0; i < odd_len - 1; i++)
    odd[i] -= ALPHA * (even[i] + even[i + 1]);
  odd[odd_len - 1] -= ALPHA * (even[odd_len - 1] + even[even_len - 1]);
}
