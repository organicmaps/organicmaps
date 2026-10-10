#include "indexer/terrain/raster_patch.hpp"

#include "indexer/terrain/sperr_decoder.hpp"

#include "coding/reader.hpp"

#include <zlib.h>
#include <iterator>
#include <limits>

namespace terrain
{
m2::RectD RasterRecord::GetRect(MeshGrid const & grid, uint8_t coordBits) const
{
  return {PointUToPointD(grid.GetPoint({m_firstColumn, m_firstRow}), coordBits),
          PointUToPointD(grid.GetPoint({m_firstColumn + m_cellsX, m_firstRow + m_cellsY}), coordBits)};
}

void RasterRecord::Validate(TwmHeader const & header, std::vector<MeshGrid> const & grids) const
{
  CHECK_LESS(m_gridIndex, grids.size(), ());
  auto const & grid = grids[m_gridIndex];
  CHECK_LESS(uint64_t{m_firstColumn} + m_cellsX, grid.m_colX.size(), ());
  CHECK_LESS(uint64_t{m_firstRow} + m_cellsY, grid.m_rowY.size(), ());
#ifdef DEBUG
  ASSERT(m_cellsX > 0 && m_cellsY > 0 && m_cellsX <= kMaxRasterCells && m_cellsY <= kMaxRasterCells,
         (m_cellsX, m_cellsY));
  ASSERT(m_codec == RasterCodec::Raw || m_codec == RasterCodec::Sperr, (static_cast<uint8_t>(m_codec)));
  ASSERT_EQUAL(m_maxErrorMm, m_codec == RasterCodec::Raw ? 0u : 3000u, ());
  ASSERT_GREATER(m_dataLength, 0, ());
  ASSERT_LESS_OR_EQUAL(m_maxErrorMm, header.m_geometries[header.m_rasterGeometryIndex].m_maxErrorMm, ());
  auto const lb = grid.GetPoint({m_firstColumn, m_firstRow});
  auto const rt = grid.GetPoint({m_firstColumn + m_cellsX, m_firstRow + m_cellsY});
  ASSERT(lb.x >= header.m_limitLB.x && lb.y >= header.m_limitLB.y && rt.x <= header.m_limitRT.x &&
             rt.y <= header.m_limitRT.y,
         ());
#else
  UNUSED_VALUE(header);
#endif
}

std::array<uint32_t, 3> RasterPatch::Triangle(uint32_t triangle) const
{
  uint32_t const cell = triangle / 2;
  uint32_t const sw = (cell / CellsX()) * (CellsX() + 1) + cell % CellsX();
  uint32_t const ne = sw + CellsX() + 2;
  if (triangle % 2 == 0)
    return {sw, sw + 1, ne};
  return {sw, ne, sw + CellsX() + 1};
}

uint32_t RasterPatch::Neighbor(uint32_t triangle, uint8_t edge) const
{
  uint32_t constexpr kNone = std::numeric_limits<uint32_t>::max();
  uint32_t const cell = triangle / 2, x = cell % CellsX(), y = cell / CellsX();
  if (triangle % 2 == 0)
  {
    if (edge == 0)
      return y == 0 ? kNone : triangle - 2 * CellsX() + 1;
    if (edge == 1)
      return x + 1 == CellsX() ? kNone : triangle + 3;
    return triangle + 1;
  }
  if (edge == 0)
    return triangle - 1;
  if (edge == 1)
    return y + 1 == CellsY() ? kNone : triangle + 2 * CellsX() - 1;
  return x == 0 ? kNone : triangle - 3;
}

std::vector<uint32_t> RasterPerimeter(uint32_t cellsX, uint32_t cellsY)
{
  ASSERT(cellsX > 0 && cellsY > 0 && cellsX <= kMaxRasterCells && cellsY <= kMaxRasterCells, ());
  uint32_t const stride = cellsX + 1;
  std::vector<uint32_t> result;
  result.reserve(2 * cellsX + 2 * cellsY);
  for (uint32_t x = 0; x <= cellsX; ++x)
    result.push_back(x);
  for (uint32_t y = 1; y <= cellsY; ++y)
    result.push_back(y * stride + cellsX);
  for (uint32_t x = cellsX; x > 0; --x)
    result.push_back(cellsY * stride + x - 1);
  for (uint32_t y = cellsY; y > 1; --y)
    result.push_back((y - 1) * stride);
  return result;
}

std::vector<Altitude> DecodeRasterPayload(RasterRecord const & record, std::span<uint8_t const> payload)
{
  CHECK(record.m_cellsX > 0 && record.m_cellsY > 0 && record.m_cellsX <= kMaxRasterCells &&
            record.m_cellsY <= kMaxRasterCells,
        (record.m_cellsX, record.m_cellsY, kMaxRasterCells));
  MemReader reader(payload.data(), payload.size());
  NonOwningReaderSource src(reader);
  std::vector<Altitude> result;
  if (record.m_codec == RasterCodec::Raw)
  {
    CHECK_EQUAL(payload.size(), record.GetNodesCount() * sizeof(Altitude), ());
    result.resize(record.GetNodesCount());
    for (auto & height : result)
      height = ReadPrimitiveFromSource<Altitude>(src);
  }
  else if (record.m_codec == RasterCodec::Sperr)
  {
    uint32_t const streamLength = ReadVarUint<uint32_t>(src);
    ASSERT_GREATER(streamLength, 0, ());
    CHECK_LESS_OR_EQUAL(streamLength, src.Size(), ());
    auto const frame = payload.subspan(src.Pos(), streamLength);
    src.Skip(streamLength);
    auto const borderEncoding = ReadPrimitiveFromSource<uint8_t>(src);
    uint32_t const borderLength = ReadVarUint<uint32_t>(src);
    CHECK_LESS_OR_EQUAL(borderLength, src.Size(), ());
    ASSERT_EQUAL(borderLength, src.Size(), ());
    ASSERT_LESS_OR_EQUAL(borderEncoding, 1, ());
    auto border = payload.subspan(src.Pos(), borderLength);
    auto const indices = RasterPerimeter(record.m_cellsX, record.m_cellsY);
    std::vector<uint8_t> inflated;
    if (borderEncoding == 1)
    {
      // Each signed int32 varint takes at most five bytes; decoding is bounded.
      inflated.resize(indices.size() * 5);
      auto length = static_cast<uLongf>(inflated.size());
      VERIFY(uncompress(inflated.data(), &length, border.data(), border.size()) == Z_OK, ());
      inflated.resize(length);
      border = inflated;
    }
    result = DecodeSperr(frame, record.m_cellsX + 1, record.m_cellsY + 1, record.m_minAltitude, record.m_maxAltitude);
    MemReader borderReader(border.data(), border.size());
    NonOwningReaderSource borderSrc(borderReader);
    int64_t height = 0;
    for (uint32_t const index : indices)
    {
      height += ReadVarInt<int32_t>(borderSrc);
      ASSERT(height >= record.m_minAltitude && height <= record.m_maxAltitude, (height));
      result[index] = static_cast<Altitude>(height);
    }
    ASSERT_EQUAL(borderSrc.Size(), 0, ());
  }
  else
    ASSERT(false, ("Unsupported raster codec", static_cast<uint8_t>(record.m_codec)));
  return result;
}
}  // namespace terrain
