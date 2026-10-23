#include "indexer/terrain/sz3_decoder.hpp"

#include "indexer/terrain/sz3_profile.hpp"
#include "indexer/terrain/terrain_serdes.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include <zlib.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace terrain
{
namespace
{
// Reused between the patches decoded on a thread.
struct Sz3Workspace
{
  SZ3::Config m_config{2, 2};
  sz3::Decomposition m_decomposition{m_config, sz3::Quantizer{}};
  SZ3::HuffmanEncoder<int> m_huffman;
  std::vector<uint8_t> m_inflated;
  std::vector<float> m_values;
  std::vector<int> m_indices;
};
}  // namespace

std::vector<Altitude> DecodeSz3(std::span<uint8_t const> frame, uint32_t nodesX, uint32_t nodesY, Altitude minimum,
                                Altitude maximum)
{
  ASSERT(nodesX >= 2 && nodesY >= 2 && nodesX <= kMaxRasterCells + 1 && nodesY <= kMaxRasterCells + 1,
         (nodesX, nodesY, kMaxRasterCells + 1));
  ASSERT_GREATER(frame.size(), 1, ());
  ASSERT_LESS_OR_EQUAL(minimum, maximum, ());
  thread_local Sz3Workspace workspace;
  auto const lossless = static_cast<sz3::FrameLossless>(frame[0]);
  auto stream = frame.subspan(1);
  if (lossless == sz3::FrameLossless::Deflate)
  {
    MemReader reader(stream.data(), stream.size());
    NonOwningReaderSource src(reader);
    uint32_t const length = ReadVarUint<uint32_t>(src);
    // A float per node plus the stream parameters and the Huffman tree bound a valid stream.
    ASSERT_LESS_OR_EQUAL(length, 8 * size_t{nodesX} * nodesY + 4096, (length, nodesX, nodesY));
    workspace.m_inflated.resize(length);
    auto inflatedLength = static_cast<uLongf>(length);
    VERIFY(uncompress(workspace.m_inflated.data(), &inflatedLength, stream.data() + src.Pos(), src.Size()) == Z_OK, ());
    ASSERT_EQUAL(inflatedLength, length, ());
    stream = workspace.m_inflated;
  }
  else
    ASSERT(lossless == sz3::FrameLossless::Bypass, (static_cast<int>(lossless)));

  auto & config = workspace.m_config;
  if (config.dims[0] != nodesY || config.dims[1] != nodesX)
  {
    std::array<size_t, 2> const dimensions{nodesY, nodesX};
    config.setDims(dimensions.begin(), dimensions.end());
  }
  // The lossless wrapper is already removed: parse the stream directly, without the Lossless_bypass copy.
  auto const * cursor = stream.data();
  size_t remaining = stream.size();
  workspace.m_decomposition.load(cursor, remaining);
  workspace.m_huffman.load(cursor, remaining);
  uint64_t count = 0;
  SZ3::read(count, cursor, remaining);
  ASSERT_EQUAL(count, config.num, (nodesX, nodesY));
  workspace.m_huffman.decode_into(cursor, count, remaining, workspace.m_indices);
  workspace.m_huffman.postprocess_decode();
  workspace.m_values.resize(config.num);
  workspace.m_decomposition.decompress(config, workspace.m_indices, workspace.m_values.data());

  std::vector<Altitude> result(config.num);
  for (size_t i = 0; i < result.size(); ++i)
  {
    float const value = workspace.m_values[i];
    ASSERT(std::isfinite(value), ());
    result[i] =
        static_cast<Altitude>(std::round(std::clamp(value, static_cast<float>(minimum), static_cast<float>(maximum))));
  }
  return result;
}
}  // namespace terrain
