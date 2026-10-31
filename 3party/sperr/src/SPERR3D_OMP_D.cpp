#include "SPERR3D_OMP_D.h"
#include "SPERR3D_Stream_Tools.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

#ifdef USE_OMP
#include <omp.h>
#endif

void sperr::SPERR3D_OMP_D::set_num_threads(size_t n)
{
#ifdef USE_OMP
  if (n == 0)
    m_num_threads = omp_get_max_threads();
  else
    m_num_threads = n;
#endif
}

auto sperr::SPERR3D_OMP_D::use_bitstream(const void* p, size_t total_len) -> RTNType
{
  // This method gathers information from the header.
  //    It does NOT, however, read the actual bitstream. The actual bitstream
  //    will be provided when the decompress() method is called.
  //
  auto tools = SPERR3D_Stream_Tools();
  auto header = tools.get_stream_header(p);

  // Verify some info.
  if (header.major_version != static_cast<uint8_t>(SPERR_VERSION_MAJOR))
    return RTNType::VersionMismatch;
  if (!header.is_3D)
    return RTNType::SliceVolumeMismatch;
  if (header.stream_len != total_len)
    return RTNType::WrongLength;

  // Collect essential info.
  m_dims = header.vol_dims;
  m_chunk_dims = header.chunk_dims;
  m_offsets = std::move(header.chunk_offsets);

  // Finally, we keep a copy of the bitstream pointer
  m_bitstream_ptr = static_cast<const uint8_t*>(p);

  return RTNType::Good;
}

auto sperr::SPERR3D_OMP_D::decompress(const void* p, bool multi_res) -> RTNType
{
  if (p == nullptr || m_bitstream_ptr == nullptr)
    return RTNType::Error;
  if (static_cast<const uint8_t*>(p) != m_bitstream_ptr)
    return RTNType::Error;
  auto eq0 = [](auto v) { return v == 0; };
  if (std::any_of(m_dims.cbegin(), m_dims.cend(), eq0) ||
      std::any_of(m_chunk_dims.cbegin(), m_chunk_dims.cend(), eq0))
    return RTNType::Error;

  // Let's figure out the chunk information
  const auto chunks = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunks.size();
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];

  // Allocate a buffer to store the entire volume
  m_vol_buf.resize(total_vals);

  // A few variables to support multi-resolution decoding.
  const auto vol_res = sperr::coarsened_resolutions(m_dims, m_chunk_dims);
  const auto chunk_res = sperr::coarsened_resolutions(m_chunk_dims);
  assert(chunk_res.size() == vol_res.size());

  // At each hierarchical level, find out where each chunk belongs to.
  auto hierarchy_chunks = std::vector<std::vector<std::array<size_t, 6>>>();
  if (multi_res) {
    m_hierarchy.resize(vol_res.size());
    hierarchy_chunks.resize(vol_res.size());
    for (size_t h = 0; h < m_hierarchy.size(); h++) {
      const auto& res = vol_res[h];
      m_hierarchy[h].resize(res[0] * res[1] * res[2]);
      hierarchy_chunks[h] = sperr::chunk_volume(res, chunk_res[h]);
    }
  }

  // Create number of decompressor instances equal to the number of threads
  auto chunk_rtn = std::vector<RTNType>(num_chunks * 2, RTNType::Good);

#ifdef USE_OMP
  m_decompressors.resize(m_num_threads);
  std::for_each(m_decompressors.begin(), m_decompressors.end(), [](auto& p) {
    if (p == nullptr)
      p = std::make_unique<SPECK3D_FLT>();
  });
#else
  if (m_decompressor == nullptr)
    m_decompressor = std::make_unique<SPECK3D_FLT>();
#endif

#pragma omp parallel for num_threads(m_num_threads)
  for (size_t chunkI = 0; chunkI < num_chunks; chunkI++) {
#ifdef USE_OMP
    auto& decompressor = m_decompressors[omp_get_thread_num()];
#else
    auto& decompressor = m_decompressor;
#endif

    // Setup decompressor parameters, and decompress!
    decompressor->set_dims({chunks[chunkI][1], chunks[chunkI][3], chunks[chunkI][5]});
    chunk_rtn[chunkI * 2] = decompressor->use_bitstream(m_bitstream_ptr + m_offsets[chunkI * 2],
                                                        m_offsets[chunkI * 2 + 1]);
    chunk_rtn[chunkI * 2 + 1] = decompressor->decompress(multi_res);
    const auto& small_vol = decompressor->view_decoded_data();
    m_scatter_chunk(m_vol_buf, m_dims, small_vol, chunks[chunkI]);

    // Also assemble the full hierarchy.
    if (multi_res) {
      const auto& low_res = decompressor->view_hierarchy();
      assert(low_res.size() == m_hierarchy.size());
      for (size_t h = 0; h < low_res.size(); h++) {
        [[maybe_unused]] const auto& small_dim = chunk_res[h];
        assert(low_res[h].size() == small_dim[0] * small_dim[1] * small_dim[2]);
        m_scatter_chunk(m_hierarchy[h], vol_res[h], low_res[h], hierarchy_chunks[h][chunkI]);
      }
    }
  }  // End of OMP parallel section.

  auto fail = std::find_if_not(chunk_rtn.begin(), chunk_rtn.end(),
                               [](auto r) { return r == RTNType::Good; });
  if (fail != chunk_rtn.end())
    return *fail;
  else
    return RTNType::Good;
}

auto sperr::SPERR3D_OMP_D::release_decoded_data() -> sperr::vecd_type&&
{
  return std::move(m_vol_buf);
}

auto sperr::SPERR3D_OMP_D::release_hierarchy() -> std::vector<vecd_type>&&
{
  return std::move(m_hierarchy);
}

auto sperr::SPERR3D_OMP_D::view_hierarchy() const -> const std::vector<vecd_type>&
{
  return m_hierarchy;
}

auto sperr::SPERR3D_OMP_D::view_decoded_data() const -> const sperr::vecd_type&
{
  return m_vol_buf;
}

auto sperr::SPERR3D_OMP_D::get_dims() const -> std::array<size_t, 3>
{
  return m_dims;
}

auto sperr::SPERR3D_OMP_D::get_chunk_dims() const -> std::array<size_t, 3>
{
  return m_chunk_dims;
}

void sperr::SPERR3D_OMP_D::m_scatter_chunk(vecd_type& big_vol,
                                           dims_type vol_dim,
                                           const vecd_type& small_vol,
                                           std::array<size_t, 6> chunk_info)
{
  size_t idx = 0;
  const auto row_len = chunk_info[1];

  for (size_t z = chunk_info[4]; z < chunk_info[4] + chunk_info[5]; z++) {
    const size_t plane_offset = z * vol_dim[0] * vol_dim[1];
    for (size_t y = chunk_info[2]; y < chunk_info[2] + chunk_info[3]; y++) {
      const auto start_i = plane_offset + y * vol_dim[0] + chunk_info[0];
      std::copy(small_vol.begin() + idx, small_vol.begin() + idx + row_len,
                big_vol.begin() + start_i);
      idx += row_len;
    }
  }
}
