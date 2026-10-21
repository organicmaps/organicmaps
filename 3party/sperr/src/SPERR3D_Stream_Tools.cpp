#include "SPERR3D_Stream_Tools.h"
#include "Conditioner.h"
#include "SPECK_INT.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <numeric>

auto sperr::SPERR3D_Stream_Tools::get_header_len(std::array<uint8_t, 20> magic) const -> size_t
{
  // Step 1: Decode the 8 booleans, and decide if there are multiple chunks.
  const auto b8 = sperr::unpack_8_booleans(magic[1]);
  const auto multi_chunk = b8[3];

  // Step 2: Extract volume and chunk dimensions
  size_t pos = 2;
  uint32_t int3[3] = {0, 0, 0};
  std::memcpy(int3, magic.data() + pos, sizeof(int3));
  pos += sizeof(int3);
  dims_type vdim = {int3[0], int3[1], int3[2]};
  dims_type cdim = {int3[0], int3[1], int3[2]};
  if (multi_chunk) {
    uint16_t short3[3] = {0, 0, 0};
    std::memcpy(short3, magic.data() + pos, sizeof(short3));
    pos += sizeof(short3);
    cdim[0] = short3[0];
    cdim[1] = short3[1];
    cdim[2] = short3[2];
  }

  // Step 3: figure out how many chunks are there, and the header length.
  auto chunks = sperr::chunk_volume(vdim, cdim);
  const auto num_chunks = chunks.size();
  assert((multi_chunk && num_chunks > 1) || (!multi_chunk && num_chunks == 1));
  size_t header_len = num_chunks * 4;
  if (multi_chunk)
    header_len += m_header_magic_nchunks;
  else
    header_len += m_header_magic_1chunk;

  return header_len;
}

auto sperr::SPERR3D_Stream_Tools::get_stream_header(const void* p) const -> SPERR3D_Header
{
  SPERR3D_Header header;
  const auto* const u8p = static_cast<const uint8_t*>(p);

  // Step 1: major version number
  header.major_version = u8p[0];
  size_t pos = 1;

  // Step 2: unpack 8 booleans.
  const auto b8 = sperr::unpack_8_booleans(u8p[pos++]);
  header.is_portion = b8[0];
  header.is_3D = b8[1];
  header.is_float = b8[2];
  header.multi_chunk = b8[3];

  // Step 3: volume and chunk dimensions
  uint32_t int3[3] = {0, 0, 0};
  std::memcpy(int3, u8p + pos, sizeof(int3));
  pos += sizeof(int3);
  header.vol_dims[0] = int3[0];
  header.vol_dims[1] = int3[1];
  header.vol_dims[2] = int3[2];
  if (header.multi_chunk) {
    uint16_t short3[3] = {0, 0, 0};
    std::memcpy(short3, u8p + pos, sizeof(short3));
    pos += sizeof(short3);
    header.chunk_dims[0] = short3[0];
    header.chunk_dims[1] = short3[1];
    header.chunk_dims[2] = short3[2];
  }
  else
    header.chunk_dims = header.vol_dims;

  auto chunks = sperr::chunk_volume(header.vol_dims, header.chunk_dims);
  const auto num_chunks = chunks.size();
  if (header.multi_chunk)
    assert(num_chunks > 1);
  else
    assert(num_chunks == 1);

  // Step 4: derived info!
  if (header.multi_chunk)
    header.header_len = m_header_magic_nchunks + num_chunks * 4;
  else
    header.header_len = m_header_magic_1chunk + num_chunks * 4;

  const auto* chunk_len = reinterpret_cast<const uint32_t*>(u8p + pos);
  header.stream_len = std::accumulate(chunk_len, chunk_len + num_chunks, header.header_len);

  header.chunk_offsets.resize(num_chunks * 2);
  header.chunk_offsets[0] = header.header_len;
  header.chunk_offsets[1] = chunk_len[0];
  for (size_t i = 1; i < num_chunks; i++) {
    header.chunk_offsets[i * 2] = header.chunk_offsets[i * 2 - 2] + header.chunk_offsets[i * 2 - 1];
    header.chunk_offsets[i * 2 + 1] = chunk_len[i];
  }

  return header;
}

auto sperr::SPERR3D_Stream_Tools::progressive_read(const std::string& filename,
                                                   unsigned pct) const -> vec8_type
{
  // Read the header of this bitstream.
  auto vec20 = sperr::read_n_bytes(filename, 20);
  if (vec20.empty())
    return vec20;
  auto arr20 = std::array<uint8_t, 20>();
  std::copy(vec20.cbegin(), vec20.cend(), arr20.begin());
  const auto header_len = this->get_header_len(arr20);
  auto header_buf = sperr::read_n_bytes(filename, header_len);
  if (header_buf.empty())
    return header_buf;

  // Get the new header and chunk offsets to read.
  auto [header_new, chunk_offsets] =
      m_progressive_helper(header_buf.data(), header_buf.size(), pct);

  // Read portions of the bitstream from disk!
  auto stream_new = std::move(header_new);
  auto rtn = sperr::read_sections(filename, chunk_offsets, stream_new);
  if (rtn != RTNType::Good)
    stream_new.clear();

  return stream_new;
}

auto sperr::SPERR3D_Stream_Tools::progressive_truncate(const void* stream,
                                                       size_t stream_len,
                                                       unsigned pct) const -> vec8_type
{
  const auto* u8p = static_cast<const uint8_t*>(stream);

  // Get the header length of this bitstream.
  assert(stream_len >= 20);
  auto arr20 = std::array<uint8_t, 20>();
  std::copy(u8p, u8p + 20, arr20.begin());
  const auto header_len = this->get_header_len(arr20);

  // Get the new header and chunk offsets to truncate.
  auto [header_new, chunk_offsets] = m_progressive_helper(stream, header_len, pct);

  // Truncate portions of the bitstream!
  auto stream_new = std::move(header_new);
  auto rtn = sperr::extract_sections(stream, stream_len, chunk_offsets, stream_new);
  if (rtn != RTNType::Good)
    stream_new.clear();

  return stream_new;
}

auto sperr::SPERR3D_Stream_Tools::m_progressive_helper(const void* header_buf,
                                                       size_t buf_len,
                                                       unsigned pct) const
    -> std::tuple<vec8_type, std::vector<size_t>>
{
  auto rtn_val = std::tuple<vec8_type, std::vector<size_t>>();
  const auto* u8p = static_cast<const uint8_t*>(header_buf);

  // Parse the header.
  //
  auto header = this->get_stream_header(header_buf);

  // If the request is beyond range, return the complete bitstream!
  //
  if (pct == 0 || pct >= 100) {
    // Copy over the header.
    std::get<0>(rtn_val).reserve(header.header_len);
    std::copy(u8p, u8p + header.header_len, std::back_inserter(std::get<0>(rtn_val)));
    // Copy over the chunks info
    std::get<1>(rtn_val) = std::move(header.chunk_offsets);
    return rtn_val;
  }

  // Calculate how many bytes to allocate to each chunk, with `m_progressive_min_chunk_bytes`
  //    being the minimal length. The only exception is that when the chunk itself has less
  //    bytes, e.g., when it's a constant chunk.
  assert(header.chunk_offsets.size() % 2 == 0);
  auto nchunks = header.chunk_offsets.size() / 2;
  for (size_t i = 0; i < nchunks; i++) {
    // Only touch the value stored at `chunk_offsets_new[i * 2 + 1]` if it's bigger than
    // the minimal number of bytes to keep.
    auto orig_len = header.chunk_offsets[i * 2 + 1];
    if (orig_len > m_progressive_min_chunk_bytes) {
      auto request_len = static_cast<size_t>(double(pct) / 100.0 * double(orig_len));
      request_len = std::max(m_progressive_min_chunk_bytes, request_len);
      header.chunk_offsets[i * 2 + 1] = request_len;
    }
  }

  // Finally, create a new header.
  //
  auto header_new = vec8_type(header.header_len);
  header_new[0] = static_cast<uint8_t>(SPERR_VERSION_MAJOR);
  size_t pos = 1;
  auto b8 = sperr::unpack_8_booleans(u8p[pos]);
  b8[0] = true;  // Record that this is a portion of another complete bitstream.
  header_new[pos++] = sperr::pack_8_booleans(b8);
  // Copy over the volume and chunk dimensions.
  if (header.multi_chunk) {
    std::copy(u8p + pos, u8p + m_header_magic_nchunks, header_new.begin() + pos);
    pos = m_header_magic_nchunks;
  }
  else {
    std::copy(u8p + pos, u8p + m_header_magic_1chunk, header_new.begin() + pos);
    pos = m_header_magic_1chunk;
  }

  // Record the length of bitstreams for each chunk.
  for (size_t i = 0; i < nchunks; i++) {
    uint32_t len = header.chunk_offsets[i * 2 + 1];
    std::memcpy(&header_new[pos], &len, sizeof(len));
    pos += sizeof(len);
  }
  assert(pos == header.header_len);
  std::get<0>(rtn_val) = std::move(header_new);
  std::get<1>(rtn_val) = std::move(header.chunk_offsets);

  return rtn_val;
}
