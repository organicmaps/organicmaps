#include "SPERR3D_OMP_C.h"

#include <algorithm>  // std::all_of()
#include <cassert>
#include <cstring>
#include <numeric>  // std::accumulate()

#ifdef USE_OMP
#include <omp.h>
#endif

void sperr::SPERR3D_OMP_C::set_num_threads(size_t n)
{
#ifdef USE_OMP
  if (n == 0)
    m_num_threads = omp_get_max_threads();
  else
    m_num_threads = n;
#endif
}

void sperr::SPERR3D_OMP_C::set_dims_and_chunks(dims_type vol_dims, dims_type chunk_dims)
{
  m_dims = vol_dims;

  // The preferred chunk size has to be between 1 and m_dims.
  for (size_t i = 0; i < m_chunk_dims.size(); i++)
    m_chunk_dims[i] = std::min(std::max(size_t{1}, chunk_dims[i]), vol_dims[i]);
}

void sperr::SPERR3D_OMP_C::set_psnr(double psnr)
{
  assert(psnr > 0.0);
  m_mode = CompMode::PSNR;
  m_quality = psnr;
}

void sperr::SPERR3D_OMP_C::set_tolerance(double pwe)
{
  assert(pwe > 0.0);
  m_mode = CompMode::PWE;
  m_quality = pwe;
}

void sperr::SPERR3D_OMP_C::set_bitrate(double bpp)
{
  assert(bpp > 0.0);
  m_mode = CompMode::Rate;
  m_quality = bpp;
}

#ifdef EXPERIMENTING
void sperr::SPERR3D_OMP_C::set_direct_q(double q)
{
  assert(q > 0.0);
  m_mode = CompMode::DirectQ;
  m_quality = q;
}
#endif

template <typename T>
auto sperr::SPERR3D_OMP_C::compress(const T* buf, size_t buf_len) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");
  if constexpr (std::is_same<T, float>::value)
    m_orig_is_float = true;
  else
    m_orig_is_float = false;

  if (m_mode == sperr::CompMode::Unknown)
    return RTNType::CompModeUnknown;
  if (buf_len != m_dims[0] * m_dims[1] * m_dims[2])
    return RTNType::WrongLength;

  // First, calculate dimensions of individual chunk indices.
  const auto chunk_idx = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunk_idx.size();

  // Let's prepare some data structures for compression!
  auto chunk_rtn = std::vector<RTNType>(num_chunks, RTNType::Good);
  m_encoded_streams.resize(num_chunks);

#ifdef USE_OMP
  m_compressors.resize(m_num_threads);
  for (auto& p : m_compressors) {
    if (p == nullptr)
      p = std::make_unique<SPECK3D_FLT>();
  }
#else
  if (m_compressor == nullptr)
    m_compressor = std::make_unique<SPECK3D_FLT>();
#endif

#pragma omp parallel for num_threads(m_num_threads)
  for (size_t i = 0; i < num_chunks; i++) {
#ifdef USE_OMP
    auto& compressor = m_compressors[omp_get_thread_num()];
#else
    auto& compressor = m_compressor;
#endif

    // Gather data for this chunk, Setup compressor parameters, and compress!
    auto chunk = m_gather_chunk<T>(buf, m_dims, chunk_idx[i]);
    assert(!chunk.empty());
    compressor->take_data(std::move(chunk));
    compressor->set_dims({chunk_idx[i][1], chunk_idx[i][3], chunk_idx[i][5]});
    switch (m_mode) {
      case CompMode::PSNR:
        compressor->set_psnr(m_quality);
        break;
      case CompMode::PWE:
        compressor->set_tolerance(m_quality);
        break;
      case CompMode::Rate:
        compressor->set_bitrate(m_quality);
        break;
#ifdef EXPERIMENTING
      case CompMode::DirectQ:
        compressor->set_direct_q(m_quality);
        break;
#endif
      default:;  // So the compiler doesn't complain about missing cases.
    }
    chunk_rtn[i] = compressor->compress();

    // Save bitstream for each chunk in `m_encoded_stream`.
    m_encoded_streams[i].clear();
    m_encoded_streams[i].reserve(128);
    compressor->append_encoded_bitstream(m_encoded_streams[i]);
  }

  auto fail = std::find_if_not(chunk_rtn.begin(), chunk_rtn.end(),
                               [](auto r) { return r == RTNType::Good; });
  if (fail != chunk_rtn.end())
    return (*fail);

  assert(std::none_of(m_encoded_streams.cbegin(), m_encoded_streams.cend(),
                      [](auto& s) { return s.empty(); }));

  return RTNType::Good;
}
template auto sperr::SPERR3D_OMP_C::compress(const float*, size_t) -> RTNType;
template auto sperr::SPERR3D_OMP_C::compress(const double*, size_t) -> RTNType;

auto sperr::SPERR3D_OMP_C::get_encoded_bitstream() const -> vec8_type
{
  auto header = m_generate_header();
  assert(!header.empty());
  auto header_size = header.size();
  auto stream_size = std::accumulate(m_encoded_streams.cbegin(), m_encoded_streams.cend(), 0lu,
                                     [](size_t a, const auto& b) { return a + b.size(); });
  header.resize(header_size + stream_size);

  auto itr = header.begin() + header_size;
  for (const auto& s : m_encoded_streams) {
    std::copy(s.cbegin(), s.cend(), itr);
    itr += s.size();
  }

  return header;
}

auto sperr::SPERR3D_OMP_C::m_generate_header() const -> sperr::vec8_type
{
  auto header = sperr::vec8_type();

  // The header would contain the following information
  //  -- a version number                     (1 byte)
  //  -- 8 booleans                           (1 byte)
  //  -- volume dimensions                    (4 x 3 = 12 bytes)
  //  -- (optional) chunk dimensions          (2 x 3 = 6 bytes)
  //  -- length of bitstream for each chunk   (4 x num_chunks)
  //
  auto chunk_idx = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunk_idx.size();
  assert(num_chunks != 0);
  if (num_chunks != m_encoded_streams.size())
    return header;
  auto header_size = size_t{0};
  if (num_chunks > 1)
    header_size = m_header_magic_nchunks + num_chunks * 4;
  else
    header_size = m_header_magic_1chunk + num_chunks * 4;

  header.resize(header_size);

  // Version number
  header[0] = static_cast<uint8_t>(SPERR_VERSION_MAJOR);
  size_t pos = 1;

  // 8 booleans:
  // bool[0]  : if this bitstream is a portion of another complete bitstream (progressive access).
  // bool[1]  : if this bitstream is for 3D (true) or 2D (false) data.
  // bool[2]  : if the original data is float (true) or double (false).
  // bool[3]  : if there are multiple chunks (true) or a single chunk (false).
  // bool[4-7]: unused
  //
  const auto b8 = std::array<bool, 8>{false,  // not a portion
                                      true,   // 3D
                                      m_orig_is_float,
                                      (num_chunks > 1),
                                      false,   // unused
                                      false,   // unused
                                      false,   // unused
                                      false};  // unused

  header[pos++] = sperr::pack_8_booleans(b8);

  // Volume dimensions
  const auto vdim = std::array{static_cast<uint32_t>(m_dims[0]), static_cast<uint32_t>(m_dims[1]),
                               static_cast<uint32_t>(m_dims[2])};
  std::memcpy(&header[pos], vdim.data(), sizeof(vdim));
  pos += sizeof(vdim);

  // Chunk dimensions, if there are more than one chunk.
  if (num_chunks > 1) {
    auto vcdim =
        std::array{static_cast<uint16_t>(m_chunk_dims[0]), static_cast<uint16_t>(m_chunk_dims[1]),
                   static_cast<uint16_t>(m_chunk_dims[2])};
    std::memcpy(&header[pos], vcdim.data(), sizeof(vcdim));
    pos += sizeof(vcdim);
  }

  // Length of bitstream for each chunk.
  for (const auto& stream : m_encoded_streams) {
    assert(stream.size() <= uint64_t{std::numeric_limits<uint32_t>::max()});
    uint32_t len = stream.size();
    std::memcpy(&header[pos], &len, sizeof(len));
    pos += sizeof(len);
  }
  assert(pos == header_size);

  return header;
}

template <typename T>
auto sperr::SPERR3D_OMP_C::m_gather_chunk(const T* vol,
                                          dims_type vol_dim,
                                          std::array<size_t, 6> chunk) -> vecd_type
{
  auto chunk_buf = vecd_type();
  if (chunk[0] + chunk[1] > vol_dim[0] || chunk[2] + chunk[3] > vol_dim[1] ||
      chunk[4] + chunk[5] > vol_dim[2])
    return chunk_buf;

  chunk_buf.resize(chunk[1] * chunk[3] * chunk[5]);
  const auto row_len = chunk[1];

  size_t idx = 0;
  for (size_t z = chunk[4]; z < chunk[4] + chunk[5]; z++) {
    const size_t plane_offset = z * vol_dim[0] * vol_dim[1];
    for (size_t y = chunk[2]; y < chunk[2] + chunk[3]; y++) {
      const auto start_i = plane_offset + y * vol_dim[0] + chunk[0];
      std::copy(vol + start_i, vol + start_i + row_len, chunk_buf.begin() + idx);
      idx += row_len;
    }
  }

  // Will be subject to Named Return Value Optimization.
  return chunk_buf;
}
template auto sperr::SPERR3D_OMP_C::m_gather_chunk(const float*,
                                                   dims_type,
                                                   std::array<size_t, 6>) -> vecd_type;
template auto sperr::SPERR3D_OMP_C::m_gather_chunk(const double*,
                                                   dims_type,
                                                   std::array<size_t, 6>) -> vecd_type;
