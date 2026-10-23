#include "indexer/terrain/sz3_decoder.hpp"

#include "indexer/terrain/sz3_profile.hpp"
#include "indexer/terrain/terrain_serdes.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include <zlib.h>

#include <algorithm>
#include <cmath>

namespace terrain
{
std::vector<Altitude> DecodeSz3(std::span<uint8_t const> frame, uint32_t nodesX, uint32_t nodesY, Altitude minimum,
                                Altitude maximum)
{
  ASSERT(nodesX >= 2 && nodesY >= 2 && nodesX <= kMaxRasterCells + 1 && nodesY <= kMaxRasterCells + 1,
         (nodesX, nodesY, kMaxRasterCells + 1));
  ASSERT_GREATER(frame.size(), 1, ());
  ASSERT_LESS_OR_EQUAL(minimum, maximum, ());
  auto const lossless = static_cast<sz3::FrameLossless>(frame[0]);
  auto stream = frame.subspan(1);
  std::vector<uint8_t> inflated;
  if (lossless == sz3::FrameLossless::Deflate)
  {
    MemReader reader(stream.data(), stream.size());
    NonOwningReaderSource src(reader);
    uint32_t const length = ReadVarUint<uint32_t>(src);
    // A float per node plus the stream parameters and the Huffman tree bound a valid stream.
    ASSERT_LESS_OR_EQUAL(length, 8 * size_t{nodesX} * nodesY + 4096, (length, nodesX, nodesY));
    inflated.resize(length);
    auto inflatedLength = static_cast<uLongf>(length);
    VERIFY(uncompress(inflated.data(), &inflatedLength, stream.data() + src.Pos(), src.Size()) == Z_OK, ());
    ASSERT_EQUAL(inflatedLength, length, ());
    stream = inflated;
  }
  else
    ASSERT(lossless == sz3::FrameLossless::Bypass, (static_cast<int>(lossless)));

  auto const conf = sz3::MakeConfig(nodesX, nodesY);
  std::vector<float> values(conf.num);
  auto compressor = sz3::MakeCompressor();
  compressor.decompress(conf, stream.data(), stream.size(), values.data());
  std::vector<Altitude> result;
  result.reserve(values.size());
  for (float const value : values)
  {
    ASSERT(std::isfinite(value), ());
    result.push_back(static_cast<Altitude>(
        std::lround(std::clamp(value, static_cast<float>(minimum), static_cast<float>(maximum)))));
  }
  return result;
}
}  // namespace terrain
