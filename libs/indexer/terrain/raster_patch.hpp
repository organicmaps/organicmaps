#pragma once

#include "indexer/terrain/terrain_serdes.hpp"

#include <array>
#include <limits>
#include <span>

namespace terrain
{
enum class RasterCodec : uint8_t
{
  Raw = 0,
  // Headerless 2D PWE, pinned SPERR profile 1, followed by sparse perimeter corrections.
  Sperr = 1,
  SperrZlibBorder = 2,  // Same SPERR frame, with zlib-compressed corrections.
  // Raster version 2: the SZ3 frame (see sz3_profile.hpp), followed by the same corrections.
  Sz3 = 3,
  Sz3ZlibBorder = 4
};

inline bool IsSz3(RasterCodec codec)
{
  return codec == RasterCodec::Sz3 || codec == RasterCodec::Sz3ZlibBorder;
}

// Compressed payload: frame length, frame, then correction bytes through the end of the record.
// Corrections: varuint count, then packed varints with delta perimeter ordinal in the high bits
// and ZigZag correction to the decoded frame in the low three bits. Previous ordinal starts at zero.

struct RasterRecord
{
  uint32_t m_gridIndex = 0;
  // Row-major ordinal in the grid interior, excluding the one-cell TIN rim.
  uint32_t m_patchIndex = 0;
  // The decoders clamp reconstructed heights to the source range before rounding.
  Altitude m_minAltitude = 0, m_maxAltitude = 0;
  RasterCodec m_codec = RasterCodec::Raw;
  uint32_t m_dataOffset = 0;
  uint32_t m_dataLength = 0;

  m2::RectU GetGridRect(MeshGrid const & grid) const;
  m2::RectD GetRect(MeshGrid const & grid, uint8_t coordBits) const;
  void Validate(TwmHeader const & header, MeshGrid const & grid) const;
};

template <typename Sink>
void SerializeRasterRecord(Sink & sink, RasterRecord const & r)
{
  WriteVarUint(sink, r.m_gridIndex);
  WriteVarUint(sink, r.m_patchIndex);
  WriteToSink(sink, static_cast<uint8_t>(r.m_codec));
  if (r.m_codec != RasterCodec::Raw)
  {
    WriteVarInt(sink, int32_t{r.m_minAltitude});
    WriteVarUint(sink, static_cast<uint32_t>(r.m_maxAltitude - r.m_minAltitude));
  }
  WriteVarUint(sink, r.m_dataOffset);
  WriteVarUint(sink, r.m_dataLength);
}

template <typename Source>
RasterRecord DeserializeRasterRecord(Source & src)
{
  RasterRecord r;
  r.m_gridIndex = ReadVarUint<uint32_t>(src);
  r.m_patchIndex = ReadVarUint<uint32_t>(src);
  r.m_codec = static_cast<RasterCodec>(ReadPrimitiveFromSource<uint8_t>(src));
  if (r.m_codec != RasterCodec::Raw)
  {
    int32_t const minimum = ReadVarInt<int32_t>(src);
    int64_t const maximum = int64_t{minimum} + ReadVarUint<uint32_t>(src);
    ASSERT_GREATER_OR_EQUAL(minimum, std::numeric_limits<Altitude>::min(), ());
    ASSERT_LESS_OR_EQUAL(maximum, std::numeric_limits<Altitude>::max(), ());
    r.m_minAltitude = static_cast<Altitude>(minimum);
    r.m_maxAltitude = static_cast<Altitude>(maximum);
  }
  r.m_dataOffset = ReadVarUint<uint32_t>(src);
  r.m_dataLength = ReadVarUint<uint32_t>(src);
  return r;
}

// Owns the frozen axes and reconstructed integer heights, independently of the
// Reader/registry handle. Rows run south to north, x fastest. Cells are owned once;
// neighboring patches duplicate only perimeter nodes.
struct RasterPatch
{
  std::vector<uint32_t> m_columns, m_rows;
  std::vector<Altitude> m_heights;

  uint32_t CellsX() const { return static_cast<uint32_t>(m_columns.size() - 1); }
  uint32_t CellsY() const { return static_cast<uint32_t>(m_rows.size() - 1); }
  m2::PointU PointU(uint32_t node) const
  {
    return {m_columns[node % m_columns.size()], m_rows[node / m_columns.size()]};
  }
  m2::PointD Point(uint32_t node) const { return PointUToPointD(PointU(node), kTerrainCoordBits); }
  m2::RectD GetRect() const { return {Point(0), Point(static_cast<uint32_t>(m_heights.size() - 1))}; }

  // CCW SW-SE-NE and SW-NE-NW, with the SW-NE diagonal fixed in every cell.
  std::array<uint32_t, 3> Triangle(uint32_t triangle) const;
  uint32_t Neighbor(uint32_t triangle, uint8_t edge) const;
};

// Perimeter indices, counterclockwise from SW, each corner exactly once.
class RasterPerimeter
{
public:
  RasterPerimeter(uint32_t cellsX, uint32_t cellsY)
    : m_cellsX(cellsX)
    , m_cellsY(cellsY)
    , m_size(2 * cellsX + 2 * cellsY)
  {
    ASSERT(cellsX > 0 && cellsY > 0 && cellsX <= kMaxRasterCells && cellsY <= kMaxRasterCells, (cellsX, cellsY));
  }

  uint32_t Size() const { return m_size; }

  uint32_t operator()(uint32_t ordinal) const;

private:
  uint32_t m_cellsX, m_cellsY;
  uint32_t const m_size;
};

std::vector<Altitude> DecodeRasterPayload(RasterRecord const & record, std::span<uint8_t const> payload,
                                          uint32_t cellsX, uint32_t cellsY);
}  // namespace terrain
