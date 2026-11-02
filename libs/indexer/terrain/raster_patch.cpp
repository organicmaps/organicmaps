#include "indexer/terrain/raster_patch.hpp"

#include "indexer/terrain/sperr_decoder.hpp"
#include "indexer/terrain/sz3_decoder.hpp"

#include "coding/reader.hpp"

#include "base/bits.hpp"
#include "base/buffer_vector.hpp"

#include <zlib.h>
#include <algorithm>
#include <limits>

namespace terrain
{
m2::RectU RasterRecord::GetGridRect(MeshGrid const & grid) const
{
  ASSERT_GREATER(grid.m_colX.size(), 3, ());
  ASSERT_GREATER(grid.m_rowY.size(), 3, ());
  auto const interiorX = static_cast<uint32_t>(grid.m_colX.size() - 3);
  auto const interiorY = static_cast<uint32_t>(grid.m_rowY.size() - 3);
  uint32_t const columns = (interiorX - 1) / kMaxRasterCells + 1;
  [[maybe_unused]] uint32_t const rows = (interiorY - 1) / kMaxRasterCells + 1;
  ASSERT_LESS(m_patchIndex, uint64_t{columns} * rows, (m_patchIndex, columns, rows));
  uint32_t const x = (m_patchIndex % columns) * kMaxRasterCells;
  uint32_t const y = (m_patchIndex / columns) * kMaxRasterCells;
  return {x + 1, y + 1, x + 1 + std::min(kMaxRasterCells, interiorX - x),
          y + 1 + std::min(kMaxRasterCells, interiorY - y)};
}

m2::RectD RasterRecord::GetRect(MeshGrid const & grid, uint8_t coordBits) const
{
  auto const rect = GetGridRect(grid);
  return {PointUToPointD(grid.GetPoint(rect.LeftBottom()), coordBits),
          PointUToPointD(grid.GetPoint(rect.RightTop()), coordBits)};
}

void RasterRecord::Validate(TwmHeader const & header, MeshGrid const & grid) const
{
#ifdef DEBUG
  ASSERT(m_codec == RasterCodec::Raw || m_codec == RasterCodec::Sperr || m_codec == RasterCodec::SperrZlibBorder ||
             (IsSz3(m_codec) && header.GetRasterVersion() >= 2),
         (static_cast<uint8_t>(m_codec), header.GetRasterVersion()));
  ASSERT_GREATER(m_dataLength, 0, ());
  auto const rect = GetGridRect(grid);
  auto const lb = grid.GetPoint(rect.LeftBottom());
  auto const rt = grid.GetPoint(rect.RightTop());
  ASSERT(lb.x >= header.m_limitLB.x && lb.y >= header.m_limitLB.y && rt.x <= header.m_limitRT.x &&
             rt.y <= header.m_limitRT.y,
         ());
#else
  UNUSED_VALUE(header);
  UNUSED_VALUE(grid);
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

uint32_t RasterPerimeter::operator()(uint32_t ordinal) const
{
  ASSERT_LESS(ordinal, Size(), ());
  uint32_t const stride = m_cellsX + 1;
  if (ordinal <= m_cellsX)
    return ordinal;
  ordinal -= m_cellsX;
  if (ordinal <= m_cellsY)
    return ordinal * stride + m_cellsX;
  ordinal -= m_cellsY;
  if (ordinal <= m_cellsX)
    return m_cellsY * stride + m_cellsX - ordinal;
  ordinal -= m_cellsX;
  return (m_cellsY - ordinal) * stride;
}

std::vector<Altitude> DecodeRasterPayload(RasterRecord const & record, std::span<uint8_t const> payload,
                                          uint32_t cellsX, uint32_t cellsY)
{
  MemReader reader(payload.data(), payload.size());
  NonOwningReaderSource src(reader);
  std::vector<Altitude> result;
  if (record.m_codec == RasterCodec::Raw)
  {
    size_t const nodes = size_t{cellsX + 1} * (cellsY + 1);
    CHECK_EQUAL(payload.size(), nodes * sizeof(Altitude), ());
    result.resize(nodes);
    for (auto & height : result)
      height = ReadPrimitiveFromSource<Altitude>(src);
  }
  else
  {
    uint32_t const streamLength = ReadVarUint<uint32_t>(src);
    ASSERT_GREATER(streamLength, 0, ());
    CHECK_LESS_OR_EQUAL(streamLength, src.Size(), ());
    auto const frame = payload.subspan(src.Pos(), streamLength);
    src.Skip(streamLength);

    auto border = payload.subspan(src.Pos());
    RasterPerimeter const perimeter(cellsX, cellsY);
    buffer_vector<uint8_t, 8 * kMaxRasterCells + 2> inflated;
    if (record.m_codec == RasterCodec::SperrZlibBorder || record.m_codec == RasterCodec::Sz3ZlibBorder)
    {
      // Each packed correction and its count take at most two bytes.
      inflated.resize(perimeter.Size() * 2 + 2);
      auto length = static_cast<uLongf>(inflated.size());
      VERIFY(uncompress(inflated.data(), &length, border.data(), border.size()) == Z_OK, ());
      inflated.resize(length);
      border = inflated;
    }
    result = IsSz3(record.m_codec)
               ? DecodeSz3(frame, cellsX + 1, cellsY + 1, record.m_minAltitude, record.m_maxAltitude)
               : DecodeSperr(frame, cellsX + 1, cellsY + 1, record.m_minAltitude, record.m_maxAltitude);

    MemReader borderReader(border.data(), border.size());
    NonOwningReaderSource borderSrc(borderReader);
    uint32_t const corrections = ReadVarUint<uint32_t>(borderSrc);
    ASSERT_LESS_OR_EQUAL(corrections, perimeter.Size(), ());
    uint64_t ordinal = 0;
    for (uint32_t i = 0; i < corrections; ++i)
    {
      uint32_t const packed = ReadVarUint<uint32_t>(borderSrc);
      uint32_t const delta = packed >> 3;
      ASSERT(i == 0 || delta > 0, (i, delta));
      ordinal += delta;
      ASSERT_LESS(ordinal, perimeter.Size(), (ordinal, perimeter.Size()));
      auto const index = perimeter(static_cast<uint32_t>(ordinal));
      int32_t const correction = bits::ZigZagDecode(packed & 7);
      ASSERT(correction != 0 && correction >= -3 && correction <= 3, (correction));
      int64_t const height = int64_t{result[index]} + correction;
      ASSERT(height >= record.m_minAltitude && height <= record.m_maxAltitude, (height));
      result[index] = static_cast<Altitude>(height);
    }
    ASSERT_EQUAL(borderSrc.Size(), 0, ());
  }
  return result;
}
}  // namespace terrain
