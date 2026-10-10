#include "sperr_helper.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numeric>

#if __cplusplus >= 202002L
#include <bit>
#endif

#ifdef USE_OMP
#include <omp.h>
#endif

auto sperr::aligned_malloc(size_t alignment, size_t size) -> void*
{
#ifdef _WIN32
  return _aligned_malloc(size, alignment);
#else
  return std::aligned_alloc(alignment, size);
#endif
}

void sperr::aligned_free(void* p)
{
#ifdef _WIN32
  _aligned_free(p);
#else
  std::free(p);
#endif
}

auto sperr::num_of_xforms(size_t len) -> size_t
{
  assert(len > 0);
  // I decide 9 is the minimal length to do one level of xform.
  // I also decide that no matter what the input size is,
  // six (6) is the maxinum number of transforms to do.
  //
  size_t num = 0;
  while (len >= 9) {
    ++num;
    len -= len / 2;
  }
  return std::min(num, size_t{6});
}

auto sperr::can_use_dyadic(dims_type dims) -> std::optional<size_t>
{
  // In case of 2D or 1D `dims`, return empty right away.
  if (dims[2] < 2 || dims[1] < 2)
    return {};

  auto xy = sperr::num_of_xforms(std::min(dims[0], dims[1]));
  auto z = sperr::num_of_xforms(dims[2]);

  // Note: if some dimensions can do 5 levels of transforms and some can do 6, we use
  //       dyanic scheme and do 5 levels on all of them. I.e., the benefit of dyanic
  //       transforms exceeds one extra level of transform.
  //
  if ((xy == z) || (xy >= 5 && z >= 5))
    return std::min(xy, z);
  else
    return {};
}

auto sperr::coarsened_resolutions(dims_type full_dims) -> std::vector<dims_type>
{
  auto resolutions = std::vector<dims_type>();

  if (full_dims[2] > 1) {  // 3D.
    const auto dyadic = sperr::can_use_dyadic(full_dims);
    if (dyadic) {
      resolutions.reserve(*dyadic);
      for (size_t lev = *dyadic; lev > 0; lev--) {
        auto [x, xd] = sperr::calc_approx_detail_len(full_dims[0], lev);
        auto [y, yd] = sperr::calc_approx_detail_len(full_dims[1], lev);
        auto [z, zd] = sperr::calc_approx_detail_len(full_dims[2], lev);
        resolutions.push_back({x, y, z});
      }
    }
  }
  else {  // 2D. Assume that there's no 1D use case that requires multi-resolution.
    size_t xy = sperr::num_of_xforms(std::min(full_dims[0], full_dims[1]));
    resolutions.reserve(xy);
    for (size_t lev = xy; lev > 0; lev--) {
      auto [x, xd] = sperr::calc_approx_detail_len(full_dims[0], lev);
      auto [y, yd] = sperr::calc_approx_detail_len(full_dims[1], lev);
      resolutions.push_back({x, y, 1});
    }
  }

  return resolutions;
}

auto sperr::coarsened_resolutions(dims_type vdim, dims_type cdim) -> std::vector<dims_type>
{
  auto resolutions = std::vector<dims_type>();

  // Test if the volume dimension is divisible by the chunk dimension.
  bool divisible = true;
  for (size_t i = 0; i < 3; i++)
    if (vdim[i] % cdim[i] != 0)
      divisible = false;

  if (divisible) {
    auto nx = vdim[0] / cdim[0];
    auto ny = vdim[1] / cdim[1];
    auto nz = vdim[2] / cdim[2];

    resolutions = sperr::coarsened_resolutions(cdim);
    for (auto& resolution : resolutions) {
      resolution[0] *= nx;
      resolution[1] *= ny;
      resolution[2] *= nz;
    }
  }

  return resolutions;
}

auto sperr::num_of_partitions(size_t len) -> size_t
{
  size_t num_of_parts = 0;  // Num. of partitions we can do
  while (len > 1) {
    num_of_parts++;
    len -= len / 2;
  }

  return num_of_parts;
}

auto sperr::calc_approx_detail_len(size_t orig_len, size_t lev) -> std::array<size_t, 2>
{
  size_t low_len = orig_len;
  size_t high_len = 0;
  for (size_t i = 0; i < lev; i++) {
    high_len = low_len / 2;
    low_len -= high_len;
  }

  return {low_len, high_len};
}

// Good solution to deal with bools and unsigned chars
// https://stackoverflow.com/questions/8461126/how-to-create-a-byte-out-of-8-bool-values-and-vice-versa
auto sperr::pack_booleans(vec8_type& dest, const std::vector<bool>& src, size_t offset) -> RTNType
{
  // `src` has to have a size of multiples of 8.
  if (src.size() % 8 != 0)
    return RTNType::WrongLength;

  // `dest` should have enough space, as the API specifies.
  assert(dest.size() >= offset + src.size() / 8);

  // How many bits to process at a time.
  constexpr uint64_t bit_stride = 2048;
  constexpr uint64_t byte_stride = bit_stride / 8;
  constexpr uint64_t magic = 0x8040201008040201;

  // It turns out C++ doesn't specify bool to be 1 byte, so we have to use
  // uint8_t here which is definitely 1 byte in size.
  // Also, C++ guarantees conversion between booleans and integers:
  // true <--> 1, false <--> 0.
  auto a = std::array<uint8_t, bit_stride>();
  auto t = std::array<uint64_t, byte_stride>();
  size_t dest_idx = offset;
  auto itr_finish = src.cbegin() + (src.size() / bit_stride) * bit_stride;

  for (auto itr = src.cbegin(); itr != itr_finish; itr += bit_stride) {
    std::copy(itr, itr + bit_stride, a.begin());
    std::memcpy(t.data(), a.data(), a.size());
    std::transform(t.cbegin(), t.cend(), dest.begin() + dest_idx,
                   [](auto e) { return (magic * e) >> 56; });
    dest_idx += byte_stride;
  }

  // For the remaining bits, process 8 bits at a time.
  for (auto itr = itr_finish; itr != src.cend(); itr += 8) {
    std::copy(itr, itr + 8, a.begin());
    std::memcpy(t.data(), a.data(), 8);
    dest[dest_idx++] = (magic * t[0]) >> 56;
  }

  return RTNType::Good;
}

auto sperr::unpack_booleans(std::vector<bool>& dest,
                            const void* src,
                            size_t src_len,
                            size_t src_offset) -> RTNType
{
  // For some reason, a strided unpack implementation similar to the striding
  // strategy used in `pack_booleans()` would result in slower runtime...
  // See Github issue #122.

  if (src == nullptr)
    return RTNType::Error;

  if (src_len < src_offset)
    return RTNType::WrongLength;

  const size_t num_of_bytes = src_len - src_offset;

  // `dest` needs to have enough space allocated, as the API specifies.
  assert(dest.size() >= num_of_bytes * 8);

  const uint8_t* src_ptr = static_cast<const uint8_t*>(src) + src_offset;
  const uint64_t magic = 0x8040201008040201;
  const uint64_t mask = 0x8080808080808080;

#ifndef OMP_UNPACK_BOOLEANS
  // Serial implementation
  //
  auto a = std::array<uint8_t, 8>();
  auto t = uint64_t{0};
  auto b8 = uint64_t{0};
  auto dest_itr = dest.begin();
  for (size_t byte_idx = 0; byte_idx < num_of_bytes; byte_idx++) {
    b8 = *(src_ptr + byte_idx);
    t = ((magic * b8) & mask) >> 7;
    std::memcpy(a.data(), &t, 8);
    std::copy(a.cbegin(), a.cend(), dest_itr);
    dest_itr += 8;
  }
#else
  // Parallel implementation
  //
  // Because in most implementations std::vector<bool> is stored as uint64_t
  // values, we parallel in strides of 64 bits, or 8 bytes.
  const size_t stride_size = 8;
  const size_t num_of_strides = num_of_bytes / stride_size;

#pragma omp parallel for
  for (size_t stride = 0; stride < num_of_strides; stride++) {
    uint8_t a[64]{0};
    for (size_t byte = 0; byte < stride_size; byte++) {
      const uint8_t* ptr = src_ptr + stride * stride_size + byte;
      const uint64_t t = ((magic * (*ptr)) & mask) >> 7;
      std::memcpy(a + byte * 8, &t, 8);
    }
    for (size_t i = 0; i < 64; i++)
      dest[stride * 64 + i] = a[i];
  }
  // This loop is at most 7 iterations, so not to worry about parallel anymore.
  for (size_t byte_idx = stride_size * num_of_strides; byte_idx < num_of_bytes; byte_idx++) {
    const uint8_t* ptr = src_ptr + byte_idx;
    const uint64_t t = ((magic * (*ptr)) & mask) >> 7;
    uint8_t a[8]{0};
    std::memcpy(a, &t, 8);
    for (size_t i = 0; i < 8; i++)
      dest[byte_idx * 8 + i] = a[i];
  }
#endif

  return RTNType::Good;
}

auto sperr::pack_8_booleans(std::array<bool, 8> src) -> uint8_t
{
  // It turns out that C++ doesn't specify bool to be one byte,
  // so to be safe we copy the content of src to array of uint8_t.
  auto bytes = std::array<uint8_t, 8>();
  std::copy(src.cbegin(), src.cend(), bytes.begin());
  const uint64_t magic = 0x8040201008040201;
  uint64_t t = 0;
  std::memcpy(&t, bytes.data(), 8);
  uint8_t dest = (magic * t) >> 56;
  return dest;
}

auto sperr::unpack_8_booleans(uint8_t src) -> std::array<bool, 8>
{
  const uint64_t magic = 0x8040201008040201;
  const uint64_t mask = 0x8080808080808080;
  uint64_t t = ((magic * src) & mask) >> 7;
  // It turns out that C++ doesn't specify bool to be one byte,
  // so to be safe we use an array of uint8_t.
  auto bytes = std::array<uint8_t, 8>();
  std::memcpy(bytes.data(), &t, 8);
  auto b8 = std::array<bool, 8>();
  std::copy(bytes.cbegin(), bytes.cend(), b8.begin());
  return b8;
}

auto sperr::read_n_bytes(std::string filename, size_t n_bytes) -> vec8_type
{
  auto buf = vec8_type();

  auto closer = [](std::FILE* f) { std::fclose(f); };  // bypass a compiler warning
  std::unique_ptr<std::FILE, decltype(closer)> fp(std::fopen(filename.data(), "rb"), closer);

  if (!fp)
    return buf;

  // POSIX systems require the size of a file to be specified, so
  // one can fseek to the end of the file.
  if (std::fseek(fp.get(), 0, SEEK_END) != 0)
    return buf;
  if (std::ftell(fp.get()) < n_bytes)
    return buf;

  std::rewind(fp.get());
  buf.resize(n_bytes);
  if (std::fread(buf.data(), 1, n_bytes, fp.get()) != n_bytes)
    buf.clear();

  return buf;
}

template <typename T>
auto sperr::read_whole_file(std::string filename) -> vec_type<T>
{
  auto buf = vec_type<T>();

  auto closer = [](std::FILE* f) { std::fclose(f); };  // bypass a compiler warning
  std::unique_ptr<std::FILE, decltype(closer)> fp(std::fopen(filename.data(), "rb"), closer);
  if (!fp)
    return buf;

  // POSIX systems require the size of a file to be specified, so
  // one can fseek to the end of the file.
  if (std::fseek(fp.get(), 0, SEEK_END) != 0)
    return buf;
  const size_t file_size = std::ftell(fp.get());
  if (file_size % sizeof(T) != 0)
    return buf;

  const size_t num_vals = file_size / sizeof(T);
  buf.resize(num_vals);
  std::rewind(fp.get());
  size_t nread = std::fread(buf.data(), sizeof(T), num_vals, fp.get());
  if (nread != num_vals)
    buf.clear();

  return buf;
}
template auto sperr::read_whole_file(std::string) -> vecf_type;
template auto sperr::read_whole_file(std::string) -> vecd_type;
template auto sperr::read_whole_file(std::string) -> vec8_type;

auto sperr::write_n_bytes(std::string filename, size_t n_bytes, const void* buffer) -> RTNType
{
  auto closer = [](std::FILE* f) { std::fclose(f); };  // bypass a compiler warning
  std::unique_ptr<std::FILE, decltype(closer)> fp(std::fopen(filename.data(), "wb"), closer);
  if (!fp)
    return RTNType::IOError;

  if (std::fwrite(buffer, 1, n_bytes, fp.get()) != n_bytes)
    return RTNType::IOError;
  else
    return RTNType::Good;
}

auto sperr::read_sections(std::string filename,
                          const std::vector<size_t>& sections,
                          vec8_type& dst) -> RTNType
{
  // Calculate the farthest file location to be read.
  size_t far = 0;
  for (size_t i = 0; i < sections.size() / 2; i++)
    far = std::max(far, sections[i * 2] + sections[i * 2 + 1]);

  // Prepare to read the file.
  auto closer = [](std::FILE* f) { std::fclose(f); };  // bypass a compiler warning
  std::unique_ptr<std::FILE, decltype(closer)> fp(std::fopen(filename.data(), "rb"), closer);
  if (!fp)
    return RTNType::IOError;

  // Retrieve the file length in bytes.
  //    P.S. POSIX systems require the size of a file to be specified, so
  //    one can fseek to the end of the file.
  auto sk = std::fseek(fp.get(), 0, SEEK_END);
  assert(sk == 0);
  const size_t file_len = std::ftell(fp.get());
  if (file_len < far)
    return RTNType::WrongLength;

  // Calculate the resulting size of `dst`, and allocate enough memory.
  auto dst_pos = dst.size();  // keep track of the current position to write section data.
  auto total_len = dst.size();
  for (size_t i = 0; i < sections.size() / 2; i++)
    total_len += sections[i * 2 + 1];
  dst.resize(total_len);

  // Read in sections of the file!
  for (size_t i = 0; i < sections.size() / 2; i++) {
    sk = std::fseek(fp.get(), sections[i * 2], SEEK_SET);
    assert(sk == 0);
    auto nread = std::fread(dst.data() + dst_pos, 1, sections[i * 2 + 1], fp.get());
    assert(nread == sections[i * 2 + 1]);
    dst_pos += nread;
  }

  return RTNType::Good;
}

auto sperr::extract_sections(const void* buf,
                             size_t buf_len,
                             const std::vector<size_t>& sections,
                             vec8_type& dst) -> RTNType
{
  // Calculate the farthest file location to be read.
  size_t far = 0;
  for (size_t i = 0; i < sections.size() / 2; i++)
    far = std::max(far, sections[i * 2] + sections[i * 2 + 1]);
  if (buf_len < far)
    return RTNType::WrongLength;

  // Calculate the resulting size of `dst`, and allocate enough memory.
  auto total_len = dst.size();
  for (size_t i = 0; i < sections.size() / 2; i++)
    total_len += sections[i * 2 + 1];
  dst.reserve(total_len);

  // Extract sections of the buffer!
  for (size_t i = 0; i < sections.size() / 2; i++) {
    const auto* beg = static_cast<const uint8_t*>(buf) + sections[i * 2];
    const auto* end = beg + sections[i * 2 + 1];
    std::copy(beg, end, std::back_inserter(dst));
  }

  return RTNType::Good;
}

template <typename T>
auto sperr::calc_stats(const T* arr1, const T* arr2, size_t arr_len, size_t omp_nthreads)
    -> std::array<T, 5>
{
  const size_t stride_size = 8192;
  const size_t num_of_strides = arr_len / stride_size;
  const size_t remainder_size = arr_len - stride_size * num_of_strides;

  // Use the maximum possible threads if 0 is passed in.
#ifdef USE_OMP
  if (omp_nthreads == 0)
    omp_nthreads = omp_get_max_threads();
#endif

  auto rmse = T{0.0};
  auto linfty = T{0.0};
  auto psnr = T{0.0};
  auto arr1min = T{0.0};
  auto arr1max = T{0.0};

  //
  // Calculate min and max of arr1
  //
  const auto minmax = std::minmax_element(arr1, arr1 + arr_len);
  arr1min = *minmax.first;
  arr1max = *minmax.second;

  //
  // In rare cases, the two input arrays are identical.
  //
  auto is_equal = std::equal(arr1, arr1 + arr_len, arr2);
  if (is_equal) {
    psnr = std::numeric_limits<T>::infinity();
    return {rmse, linfty, psnr, arr1min, arr1max};
  }

  auto sum_vec = vec_type<T>(num_of_strides + 1);
  auto linfty_vec = vec_type<T>(num_of_strides + 1);

//
// Calculate diff summation and l-infty of each stride
//
#pragma omp parallel for num_threads(omp_nthreads)
  for (size_t stride_i = 0; stride_i < num_of_strides; stride_i++) {
    T maxerr = 0.0;
    auto buf = std::array<T, stride_size>();
    for (size_t i = 0; i < stride_size; i++) {
      const size_t idx = stride_i * stride_size + i;
      auto diff = std::abs(arr1[idx] - arr2[idx]);
      maxerr = std::max(maxerr, diff);
      buf[i] = diff * diff;
    }
    sum_vec[stride_i] = std::accumulate(buf.cbegin(), buf.cend(), T{0.0});
    linfty_vec[stride_i] = maxerr;
  }

  //
  // Calculate diff summation and l-infty of the remaining elements
  //
  T last_linfty = 0.0;
  auto last_buf = std::array<T, stride_size>{};  // must be enough for `remainder_size` elements.
  for (size_t i = 0; i < remainder_size; i++) {
    const size_t idx = stride_size * num_of_strides + i;
    auto diff = std::abs(arr1[idx] - arr2[idx]);
    last_linfty = std::max(last_linfty, diff);
    last_buf[i] = diff * diff;
  }
  sum_vec[num_of_strides] = T{0.0};
  sum_vec[num_of_strides] =
      std::accumulate(last_buf.cbegin(), last_buf.cbegin() + remainder_size, T{0.0});
  linfty_vec[num_of_strides] = last_linfty;

  //
  // Now calculate linfty
  //
  linfty = *(std::max_element(linfty_vec.cbegin(), linfty_vec.cend()));

  //
  // Now calculate rmse and psnr
  // Note: psnr is calculated in dB, and follows the equation described in:
  // http://homepages.inf.ed.ac.uk/rbf/CVonline/LOCAL_COPIES/VELDHUIZEN/node18.html
  // Also refer to https://www.mathworks.com/help/vision/ref/psnr.html
  //
  const auto mse = std::accumulate(sum_vec.cbegin(), sum_vec.cend(), T{0.0}) / T(arr_len);
  rmse = std::sqrt(mse);
  const auto range_sq = (arr1max - arr1min) * (arr1max - arr1min);
  psnr = std::log10(range_sq / mse) * T{10.0};

  return {rmse, linfty, psnr, arr1min, arr1max};
}
template auto sperr::calc_stats(const float*, const float*, size_t, size_t) -> std::array<float, 5>;
template auto sperr::calc_stats(const double*,
                                const double*,
                                size_t,
                                size_t) -> std::array<double, 5>;

template <typename T>
auto sperr::kahan_summation(const T* arr, size_t len) -> T
{
  T sum = 0.0, c = 0.0;
  T t, y;
  for (size_t i = 0; i < len; i++) {
    y = arr[i] - c;
    t = sum + y;
    c = (t - sum) - y;
    sum = t;
  }

  return sum;
}
template auto sperr::kahan_summation(const float*, size_t) -> float;
template auto sperr::kahan_summation(const double*, size_t) -> double;

auto sperr::chunk_volume(dims_type vol_dim,
                         dims_type chunk_dim) -> std::vector<std::array<size_t, 6>>
{
  // Step 1: figure out how many segments are there along each axis.
  auto n_segs = std::array<size_t, 3>();
  for (size_t i = 0; i < 3; i++) {
    n_segs[i] = vol_dim[i] / chunk_dim[i];
    // In case the last segment is shorter than `chunk_dim[i]`, test if it's
    // longer than half of `chunk_dim[i]`. If it is, it can qualify another segment.
    if ((vol_dim[i] % chunk_dim[i]) > (chunk_dim[i] / 2))
      n_segs[i]++;
    // In case the volume has one dimension that's too small, let's make it
    // at least one segment anyway.
    if (n_segs[i] == 0)
      n_segs[i] = 1;
  }

  // Step 2: calculate the starting indices of each segment along each axis.
  auto x_tics = std::vector<size_t>(n_segs[0] + 1);
  for (size_t i = 0; i < n_segs[0]; i++)
    x_tics[i] = i * chunk_dim[0];
  x_tics[n_segs[0]] = vol_dim[0];  // the last tic is the length in X

  auto y_tics = std::vector<size_t>(n_segs[1] + 1);
  for (size_t i = 0; i < n_segs[1]; i++)
    y_tics[i] = i * chunk_dim[1];
  y_tics[n_segs[1]] = vol_dim[1];  // the last tic is the length in Y

  auto z_tics = std::vector<size_t>(n_segs[2] + 1);
  for (size_t i = 0; i < n_segs[2]; i++)
    z_tics[i] = i * chunk_dim[2];
  z_tics[n_segs[2]] = vol_dim[2];  // the last tic is the length in Z

  // Step 3: fill in details of each chunk
  auto n_chunks = n_segs[0] * n_segs[1] * n_segs[2];
  auto chunks = std::vector<std::array<size_t, 6>>(n_chunks);
  size_t chunk_idx = 0;
  for (size_t z = 0; z < n_segs[2]; z++)
    for (size_t y = 0; y < n_segs[1]; y++)
      for (size_t x = 0; x < n_segs[0]; x++) {
        chunks[chunk_idx][0] = x_tics[x];                  // X start
        chunks[chunk_idx][1] = x_tics[x + 1] - x_tics[x];  // X length
        chunks[chunk_idx][2] = y_tics[y];                  // Y start
        chunks[chunk_idx][3] = y_tics[y + 1] - y_tics[y];  // Y length
        chunks[chunk_idx][4] = z_tics[z];                  // Z start
        chunks[chunk_idx][5] = z_tics[z + 1] - z_tics[z];  // Z length
        chunk_idx++;
      }

  return chunks;
}

template <typename T>
auto sperr::calc_mean_var(const T* arr, size_t len, size_t omp_nthreads) -> std::array<T, 2>
{
  if (len == 0) {
    static_assert(std::is_floating_point_v<T>);
    if constexpr (std::is_same_v<T, float>)
      return {std::nanf("1"), std::nanf("2")};
    else
      return {std::nan("1"), std::nan("2")};
  }

#ifdef USE_OMP
  if (omp_nthreads == 0)
    omp_nthreads = omp_get_max_threads();
#endif

  const size_t stride_size = 16'384;
  const size_t num_strides = len / stride_size;
  auto tmp_buf = vec_type<T>(num_strides + 1);

  // First, calculate the mean of this array.
#pragma omp parallel for num_threads(omp_nthreads)
  for (size_t i = 0; i < num_strides; i++) {
    const T* beg = arr + i * stride_size;
    tmp_buf[i] = std::accumulate(beg, beg + stride_size, T{0.0});
  }
  tmp_buf[num_strides] = 0.0;
  tmp_buf[num_strides] = std::accumulate(arr + num_strides * stride_size, arr + len, T{0.0});
  const auto elem_sum = std::accumulate(tmp_buf.cbegin(), tmp_buf.cend(), T{0.0});
  const auto mean = elem_sum / static_cast<T>(len);

  // Second, calculate the variance.
#pragma omp parallel for num_threads(omp_nthreads)
  for (size_t i = 0; i < num_strides; i++) {
    const T* beg = arr + i * stride_size;
    tmp_buf[i] = std::accumulate(beg, beg + stride_size, T{0.0}, [mean](auto init, auto v) {
      return init + (v - mean) * (v - mean);
    });
  }
  tmp_buf[num_strides] = 0.0;
  tmp_buf[num_strides] =
      std::accumulate(arr + num_strides * stride_size, arr + len, T{0.0},
                      [mean](auto init, auto v) { return init + (v - mean) * (v - mean); });
  const auto diff_sum = std::accumulate(tmp_buf.cbegin(), tmp_buf.cend(), T{0.0});
  const auto var = diff_sum / static_cast<T>(len);

  return {mean, var};
}
template auto sperr::calc_mean_var(const float*, size_t, size_t) -> std::array<float, 2>;
template auto sperr::calc_mean_var(const double*, size_t, size_t) -> std::array<double, 2>;

template <typename T>
auto sperr::msb_position(T v) -> int8_t
{
#if __cplusplus >= 202002L
  return static_cast<int8_t>(sizeof(T) * 8 - 1 - std::countl_zero(v));
#else
  int8_t pos = -1;
  while (v) {
    v >>= 1;
    pos++;
  }
  return pos;
#endif
}
template auto sperr::msb_position(uint8_t) -> int8_t;
template auto sperr::msb_position(uint16_t) -> int8_t;
template auto sperr::msb_position(uint32_t) -> int8_t;
template auto sperr::msb_position(uint64_t) -> int8_t;
