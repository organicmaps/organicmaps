#include "indexer/terrain/sperr_decoder.hpp"

#include "indexer/terrain/terrain_serdes.hpp"

#include <SPECK2D_FLT.h>

#include <algorithm>
#include <cmath>

namespace terrain
{
std::vector<Altitude> DecodeSperr(std::span<uint8_t const> frame, uint32_t nodesX, uint32_t nodesY, Altitude minimum,
                                  Altitude maximum)
{
  CHECK(nodesX >= 2 && nodesY >= 2 && nodesX <= kMaxRasterCells + 1 && nodesY <= kMaxRasterCells + 1,
        (nodesX, nodesY, kMaxRasterCells + 1));
  ASSERT(!frame.empty(), ());
  ASSERT_LESS_OR_EQUAL(minimum, maximum, ());
  // No decoder or scratch is shared across const Reader requests.
  sperr::SPECK2D_FLT decoder;
  decoder.set_dims({nodesX, nodesY, 1});
  VERIFY(decoder.use_bitstream(frame.data(), frame.size()) == sperr::RTNType::Good, ());
  VERIFY(decoder.decompress() == sperr::RTNType::Good, ());
  auto const & values = decoder.view_decoded_data();
  ASSERT_EQUAL(values.size(), size_t{nodesX} * nodesY, ());
  std::vector<Altitude> result;
  result.reserve(values.size());
  for (double value : values)
  {
    // Match the float-output C API profile before rounding half away from zero.
    value = static_cast<float>(value);
    ASSERT(std::isfinite(value), ());
    result.push_back(static_cast<Altitude>(
        std::lround(std::clamp(value, static_cast<double>(minimum), static_cast<double>(maximum)))));
  }
  return result;
}
}  // namespace terrain
