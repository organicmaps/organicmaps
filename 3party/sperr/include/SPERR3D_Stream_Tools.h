#ifndef SPERR3D_STREAM_TOOLS_H
#define SPERR3D_STREAM_TOOLS_H

#include "sperr_helper.h"

namespace sperr {

//
// The 3D SPERR header definition is in SPERR3D_OMP_C.cpp::m_generate_header().
//
struct SPERR3D_Header {
  // Info directly stored in the header
  uint8_t major_version = 0;
  bool is_portion = false;
  bool is_3D = false;
  bool is_float = false;
  bool multi_chunk = false;
  dims_type vol_dims = {0, 0, 0};
  dims_type chunk_dims = {0, 0, 0};

  // Info calculated from above
  size_t header_len = 0;
  size_t stream_len = 0;
  std::vector<size_t> chunk_offsets;
};

class SPERR3D_Stream_Tools {
 public:
  // Read the first 20 bytes of a bitstream, and determine the total length of the header.
  // Need 20 bytes because it's the larger of the header magic number (in multi-chunk case).
  auto get_header_len(std::array<uint8_t, 20>) const -> size_t;

  // Read a bitstream that's at least as long as what's determined by `get_header_len()`, and
  // return an object of `SPERR3D_Stream_Header`.
  auto get_stream_header(const void*) const -> SPERR3D_Header;

  // Function that reads in portions of a file only to facilitate progressive access.
  // (This function does not read the whole file.)
  auto progressive_read(const std::string& filename, unsigned pct) const -> vec8_type;

  // Function that truncates a bitstream in the memory to facilitate progressive access.
  //    Note on `stream_len`: it does not need to be the full length of the original bitstream,
  //    rather it can be just long enough for the requested truncation:
  //  - one chunk: (full_bitstream_length * percent + 64) bytes.
  //  - multiple chunks: probably easier to just use the full bitstream length.
  auto progressive_truncate(const void* stream, size_t stream_len, unsigned pct) const -> vec8_type;

 private:
  const size_t m_header_magic_nchunks = 20;
  const size_t m_header_magic_1chunk = 14;

  // To simplify logic with progressive read, we set a minimum number of bytes to read from
  // a chunk, unless the chunk doesn't have that many bytes (e.g., a constant chunk).
  const size_t m_progressive_min_chunk_bytes = 64;

  // Given the header of a bitstream and a desired percentage to truncate, return an
  //    updated header and a list of {offset, len} to access.
  //    Note: this function assumes that the header is complete.
  auto m_progressive_helper(const void* header_buf,
                            size_t buf_len,
                            unsigned pct) const -> std::tuple<vec8_type, std::vector<size_t>>;
};

}  // End of namespace sperr

#endif
