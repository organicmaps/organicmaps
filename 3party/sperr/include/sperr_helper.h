#ifndef SPERR_HELPER_H
#define SPERR_HELPER_H

//
// We put common functions that are used across the project here.
//

#include <array>
#include <cstddef>  // std::size_t
#include <cstdint>  // fixed width integers
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#ifndef USE_VANILLA_CONFIG
#include "SperrConfig.h"
#endif

namespace sperr {

using std::size_t;  // Seems most appropriate

//
// A few shortcuts
//
template <typename T>
using vec_type = std::vector<T>;
using vecd_type = vec_type<double>;
using vecf_type = vec_type<float>;
using vec8_type = vec_type<uint8_t>;
using dims_type = std::array<size_t, 3>;

//
// Helper classes
//
enum class SigType : unsigned char { Insig, Sig, Dunno, Garbage };

enum class UINTType : unsigned char { UINT8, UINT16, UINT32, UINT64 };

enum class CompMode : unsigned char {
  PSNR,
  PWE,
  Rate,
#ifdef EXPERIMENTING
  DirectQ,
#endif
  Unknown
};

enum class RTNType {
  Good = 0,
  WrongLength,
  IOError,
  BitBudgetMet,
  VersionMismatch,
  SliceVolumeMismatch,
  CompModeUnknown,
  FE_Invalid,  // floating point exception: FE_INVALID
  Error
};

//
// Helper functions
//

// Allocate and deallocate a chunk of ALIGNED memory, for both UNIX and Windows.
auto aligned_malloc(size_t alignment, size_t size) -> void*;
void aligned_free(void* p);

// Given a certain length, how many transforms to be performed?
auto num_of_xforms(size_t len) -> size_t;

// Given a 3D dimension, tell if it can use dyadic decomposition, or wavelet-packet only.
//    I.e., will the dimension result in the same levels of wavelet decomposition in 3 directions.
//    If dyadic decomposition can be used, it returns the number of decomposition levels.
//    Otherwise, it returns an empty optional.
//    For 1D or 2D dimensions, it always returns an empty optional.
auto can_use_dyadic(dims_type) -> std::optional<size_t>;

// Given the native resolution of either a 3D volume or 2D slice, it decides if and how many
//    coarsened resolutions are available.
//    If multi-resolution is not supported, then it returns an empty vector.
//    If multi-resolution is supported, then it returns the coarsened resolutions.
//    Note 1: this function assumes a single chunk.
//    Note 2: it's UB if `dim` is a 1D array.
auto coarsened_resolutions(dims_type dim) -> std::vector<dims_type>;

// Given the native resolution and preferred chunk size of a 3D volume, it decides if and how many
//    coarsened resolutions are available.
//    If multi-resolution is not supported, then it returns an empty vector.
//    If multi-resolution is supported, then it returns the coarsened resolutions.
//    Note1 : for the multi-chunk volume to support multi-resolution,
//            1) the volume dimension has to be perfectly divisible by the chunk dimension, and
//            2) the chunk dimension has to support multi-resolution.
//    Note 2: it's UB if `vol` is a 1D array or 2D slice.
auto coarsened_resolutions(dims_type vol, dims_type chunk) -> std::vector<dims_type>;

// How many partition operation could we perform given a length?
// Length 0 and 1 can do 0 partitions; len=2 can do 1; len=3 can do 2, len=4 can
// do 2, etc.
auto num_of_partitions(size_t len) -> size_t;

// Determine the approximation and detail signal length at a certain
// transformation level lev: 0 <= lev < num_of_xforms.
// It puts the approximation and detail length as the 1st and 2nd
// element of the return array.
auto calc_approx_detail_len(size_t orig_len, size_t lev) -> std::array<size_t, 2>;

// Pack and unpack booleans to array of chars.
// When packing, the caller should make sure the number of booleans is a
// multiplier of 8. It optionally takes in an offset that specifies where to
// start writing/reading the char array.
//
// Note 1: unpack_booleans() takes a raw pointer because it accesses memory
//         provided by others, and others most likely provide it by raw pointers.
// Note 2: these two methods only work on little endian machines.
// Note 3: the caller should have already allocated enough space for `dest`.
auto pack_booleans(vec8_type& dst, const std::vector<bool>& src, size_t dest_offset = 0) -> RTNType;
auto unpack_booleans(std::vector<bool>& dest,
                     const void* src,
                     size_t src_len,
                     size_t src_offset = 0) -> RTNType;

// Pack and unpack exactly 8 booleans to/from a single byte
// Note 1: memory for the 8 booleans should already be allocated!
// Note 2: these two methods only work on little endian machines.
auto pack_8_booleans(std::array<bool, 8>) -> uint8_t;
auto unpack_8_booleans(uint8_t) -> std::array<bool, 8>;

// Read from and write to a file
// Note: not using references for `filename` to allow a c-style string literal to be passed in.
auto write_n_bytes(std::string filename, size_t n_bytes, const void* buffer) -> RTNType;
auto read_n_bytes(std::string filename, size_t n_bytes) -> vec8_type;
template <typename T>
auto read_whole_file(std::string filename) -> vec_type<T>;

// Read sections of a file (extract sections from a memory buffer), and append those sections
//    to the end of `dst`. The read from file version avoids reading not-requested sections.
//    The sections are defined by pairs of offsets and lengths, both in number of bytes.
auto read_sections(std::string filename,
                   const std::vector<size_t>& sections,
                   vec8_type& dst) -> RTNType;
auto extract_sections(const void* buf,
                      size_t buf_len,
                      const std::vector<size_t>& sections,
                      vec8_type& dst) -> RTNType;

// Calculate a suite of statistics.
//    Note that arr1 is considered as the ground truth array, so it's the range of
//    arr1 that is used internally for psnr calculations.
//    If `omp_nthreads` is zero, then it will use the maximum number of threads.
//    The return array contains statistics in the following order:
//    ret[0] : RMSE
//    ret[1] : L-Infinity
//    ret[2] : PSNR
//    ret[3] : min of arr1
//    ret[4] : max of arr1
template <typename T>
auto calc_stats(const T* arr1, const T* arr2, size_t arr_len, size_t omp_nthreads = 0)
    -> std::array<T, 5>;

template <typename T>
auto kahan_summation(const T*, size_t) -> T;

// Returns the bit position of the most significant bit (0-based), or -1 for zero.
template <typename T>
auto msb_position(T v) -> int8_t;

// Given a whole volume size and a desired chunk size, this helper function
// returns a list of chunks specified by 6 integers:
// chunk[0], [2], [4]: starting index of this chunk in X, Y, and Z;
// chunk[1], [3], [5]: length of this chunk in X, Y, and Z.
// Note 1: the values in `chunk_dim` is only suggestive, meaning that when the volume
//         dimension is not exact multiplies of requested chunk dimension,
//         approximate values are used.
// Note 2: this function works on degraded 2D or 1D volumes too.
auto chunk_volume(dims_type vol_dim, dims_type chunk_dim) -> std::vector<std::array<size_t, 6>>;

// Calculate the mean and variance of a given array.
// In case of arrays of size zero, it will return {NaN, NaN}.
// In case of `omp_nthreads == 0`, it will use all available OpenMP threads.
// ret[0] : mean
// ret[1] : variance
template <typename T>
auto calc_mean_var(const T*, size_t len, size_t omp_nthreads = 0) -> std::array<T, 2>;

}  // namespace sperr

#endif
