#include <cassert>

#include "SPERR_C_API.h"

#include "SPECK2D_FLT.h"
#include "SPERR3D_OMP_C.h"
#include "SPERR3D_OMP_D.h"

#include "SPERR3D_Stream_Tools.h"

auto C_API::sperr_comp_2d(const void* src,
                          int is_float,
                          size_t dimx,
                          size_t dimy,
                          int mode,
                          double quality,
                          int out_inc_header,
                          void** dst,
                          size_t* dst_len) -> int
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != nullptr)
    return 1;
  if (quality <= 0.0)
    return 2;

  // The actual encoding steps are just the same as in `utilities/sperr2d.cpp`.
  auto encoder = std::make_unique<sperr::SPECK2D_FLT>();
  encoder->set_dims({dimx, dimy, 1});
  if (is_float)
    encoder->copy_data(static_cast<const float*>(src), dimx * dimy);
  else
    encoder->copy_data(static_cast<const double*>(src), dimx * dimy);

  switch (mode) {
    case 1:  // fixed bitrate
      encoder->set_bitrate(quality);
      break;
    case 2:  // fixed PSNR
      encoder->set_psnr(quality);
      break;
    case 3:  // fixed PWE
      encoder->set_tolerance(quality);
      break;
    default:
      return 2;
  }
  auto rtn = encoder->compress();
  if (rtn != sperr::RTNType::Good)
    return -1;

  auto stream = sperr::vec8_type();
  if (out_inc_header) {  // Assemble a header that's the same as the header in SPERR3D_OMP_C().
    // The header would contain the following information
    //  -- a version number                     (1 byte)
    //  -- 8 booleans                           (1 byte)
    //  -- slice dimensions                     (4 x 2 = 8 bytes)
    //

    // Version number
    stream.resize(10);
    stream[0] = static_cast<uint8_t>(SPERR_VERSION_MAJOR);

    // 8 booleans:
    // bool[0]  : if this bitstream is a portion of another complete bitstream (progressive access).
    // bool[1]  : if this bitstream is for 3D (true) or 2D (false) data.
    // bool[2]  : if the original data is float (true) or double (false).
    // bool[3-7]: unused
    //
    const auto b8 = std::array{false,  // not a portion
                               false,  // 2D
                               bool(is_float),
                               false,   // unused
                               false,   // unused
                               false,   // unused
                               false,   // unused
                               false};  // unused
    stream[1] = sperr::pack_8_booleans(b8);

    // Slice dimension
    auto dims = std::array{static_cast<uint32_t>(dimx), static_cast<uint32_t>(dimy)};
    std::memcpy(stream.data() + 2, dims.data(), sizeof(dims));
  }

  // Append the actual SPERR bitstream.
  encoder->append_encoded_bitstream(stream);
  encoder.reset();  // Free up some memory.

  // Allocate buffer and copy over the content of stream.
  *dst_len = stream.size();
  auto* buf = (uint8_t*)std::malloc(*dst_len);
  std::copy(stream.cbegin(), stream.cend(), buf);
  *dst = buf;

  return 0;
}

auto C_API::sperr_decomp_2d(const void* src,
                            size_t src_len,
                            int output_float,
                            size_t dimx,
                            size_t dimy,
                            void** dst) -> int
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != nullptr)
    return 1;

  // Use a decoder, similar steps as in `utilities/sperr2d.cpp`.
  auto decoder = std::make_unique<sperr::SPECK2D_FLT>();
  decoder->set_dims({dimx, dimy, 1});
  decoder->use_bitstream(src, src_len);
  auto rtn = decoder->decompress();
  if (rtn != sperr::RTNType::Good)
    return -1;
  auto outputd = decoder->release_decoded_data();
  assert(outputd.size() == size_t{dimx} * dimy);
  decoder.reset();

  // Provide decompressed data to `dst`.
  if (output_float) {
    auto* buf = (float*)std::malloc(outputd.size() * sizeof(float));
    std::copy(outputd.cbegin(), outputd.cend(), buf);
    *dst = buf;
  }
  else {  // double
    auto* buf = (double*)std::malloc(outputd.size() * sizeof(double));
    std::copy(outputd.cbegin(), outputd.cend(), buf);
    *dst = buf;
  }

  return 0;
}

void C_API::sperr_parse_header(const void* src,
                               size_t* dimx,
                               size_t* dimy,
                               size_t* dimz,
                               int* is_float)
{
  const auto* srcp = static_cast<const uint8_t*>(src);
  const auto b8 = sperr::unpack_8_booleans(srcp[1]);
  auto is_3d = b8[1];
  *is_float = int(b8[2]);

  auto dims = std::array<uint32_t, 3>{1, 1, 1};
  if (is_3d)
    std::memcpy(dims.data(), srcp + 2, sizeof(uint32_t) * 3);
  else
    std::memcpy(dims.data(), srcp + 2, sizeof(uint32_t) * 2);
  *dimx = dims[0];
  *dimy = dims[1];
  *dimz = dims[2];
}

auto C_API::sperr_comp_3d(const void* src,
                          int is_float,
                          size_t dimx,
                          size_t dimy,
                          size_t dimz,
                          size_t chunk_x,
                          size_t chunk_y,
                          size_t chunk_z,
                          int mode,
                          double quality,
                          size_t nthreads,
                          void** dst,
                          size_t* dst_len) -> int
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != nullptr)
    return 1;
  if (quality <= 0.0)
    return 2;

  const auto dims = sperr::dims_type{dimx, dimy, dimz};
  const auto chunks = sperr::dims_type{chunk_x, chunk_y, chunk_z};

  // Setup the compressor. Very similar steps as in `utilities/sperr3d.cpp`.
  const auto total_vals = dimx * dimy * dimz;
  auto encoder = std::make_unique<sperr::SPERR3D_OMP_C>();
  encoder->set_dims_and_chunks(dims, chunks);
  encoder->set_num_threads(nthreads);
  switch (mode) {
    case 1:  // fixed bitrate
      encoder->set_bitrate(quality);
      break;
    case 2:  // fixed PSNR
      encoder->set_psnr(quality);
      break;
    case 3:  // fixed PWE
      encoder->set_tolerance(quality);
      break;
    default:
      return 2;
  }
  auto rtn = sperr::RTNType::Good;
  if (is_float)
    rtn = encoder->compress(static_cast<const float*>(src), total_vals);
  else  // double
    rtn = encoder->compress(static_cast<const double*>(src), total_vals);
  if (rtn != sperr::RTNType::Good)
    return -1;

  // Prepare the compressed bitstream.
  auto stream = encoder->get_encoded_bitstream();
  if (stream.empty())
    return -1;
  encoder.reset();
  *dst_len = stream.size();
  auto* buf = (uint8_t*)std::malloc(stream.size());
  std::copy(stream.cbegin(), stream.cend(), buf);
  *dst = buf;

  return 0;
}

auto C_API::sperr_decomp_3d(const void* src,
                            size_t src_len,
                            int output_float,
                            size_t nthreads,
                            size_t* dimx,
                            size_t* dimy,
                            size_t* dimz,
                            void** dst) -> int
{
  // Examine if `dst` is pointing to a NULL pointer.
  if (*dst != nullptr)
    return 1;

  // Use a decompressor to decompress this bitstream
  auto decoder = std::make_unique<sperr::SPERR3D_OMP_D>();
  decoder->set_num_threads(nthreads);
  decoder->use_bitstream(src, src_len);
  auto rtn = decoder->decompress(src);
  if (rtn != sperr::RTNType::Good)
    return -1;
  auto dims = decoder->get_dims();
  auto outputd = decoder->release_decoded_data();
  decoder.reset();

  // Provide the decompressed volume.
  *dimx = dims[0];
  *dimy = dims[1];
  *dimz = dims[2];
  if (output_float) {
    auto* buf = (float*)std::malloc(outputd.size() * sizeof(float));
    std::copy(outputd.cbegin(), outputd.cend(), buf);
    *dst = buf;
  }
  else {  // double
    auto* buf = (double*)std::malloc(outputd.size() * sizeof(double));
    std::copy(outputd.cbegin(), outputd.cend(), buf);
    *dst = buf;
  }

  return 0;
}

auto C_API::sperr_trunc_3d(const void* src,
                           size_t src_len,
                           unsigned pct,
                           void** dst,
                           size_t* dst_len) -> int
{
  if (*dst != nullptr)
    return 1;

  auto tools = sperr::SPERR3D_Stream_Tools();
  auto trunc = tools.progressive_truncate(src, src_len, pct);
  if (trunc.empty())
    return -1;
  else {
    auto* buf = (uint8_t*)std::malloc(trunc.size());
    std::copy(trunc.cbegin(), trunc.cend(), buf);
    *dst = buf;
    *dst_len = trunc.size();
    return 0;
  }
}
