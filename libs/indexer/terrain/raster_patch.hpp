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
  Sperr = 1  // Headerless 2D PWE, pinned SPERR profile 1.
};

struct RasterRecord
{
  uint32_t m_gridIndex = 0;
  uint32_t m_firstColumn = 0, m_firstRow = 0;
  uint32_t m_cellsX = 0, m_cellsY = 0;
  Altitude m_minAltitude = 0, m_maxAltitude = 0;
  uint32_t m_maxErrorMm = 3000;
  RasterCodec m_codec = RasterCodec::Raw;
  uint64_t m_dataOffset = 0;
  uint32_t m_dataLength = 0;

  size_t GetNodesCount() const { return size_t{m_cellsX + 1} * (m_cellsY + 1); }
  m2::RectD GetRect(MeshGrid const & grid, uint8_t coordBits) const;
  void Validate(TwmHeader const & header, std::vector<MeshGrid> const & grids) const;
};

template <typename Sink>
void SerializeRasterRecord(Sink & sink, RasterRecord const & r)
{
  WriteVarUint(sink, r.m_gridIndex);
  WriteVarUint(sink, r.m_firstColumn);
  WriteVarUint(sink, r.m_firstRow);
  WriteVarUint(sink, r.m_cellsX);
  WriteVarUint(sink, r.m_cellsY);
  WriteVarInt(sink, int32_t{r.m_minAltitude});
  WriteVarUint(sink, static_cast<uint32_t>(r.m_maxAltitude - r.m_minAltitude));
  WriteVarUint(sink, r.m_maxErrorMm);
  WriteToSink(sink, static_cast<uint8_t>(r.m_codec));
  WriteVarUint(sink, r.m_dataOffset);
  WriteVarUint(sink, r.m_dataLength);
}

template <typename Source>
RasterRecord DeserializeRasterRecord(Source & src)
{
  RasterRecord r;
  r.m_gridIndex = ReadVarUint<uint32_t>(src);
  r.m_firstColumn = ReadVarUint<uint32_t>(src);
  r.m_firstRow = ReadVarUint<uint32_t>(src);
  r.m_cellsX = ReadVarUint<uint32_t>(src);
  r.m_cellsY = ReadVarUint<uint32_t>(src);
  int32_t const minimum = ReadVarInt<int32_t>(src);
  int64_t const maximum = int64_t{minimum} + ReadVarUint<uint32_t>(src);
  ASSERT_GREATER_OR_EQUAL(minimum, std::numeric_limits<Altitude>::min(), ());
  ASSERT_LESS_OR_EQUAL(maximum, std::numeric_limits<Altitude>::max(), ());
  r.m_minAltitude = static_cast<Altitude>(minimum);
  r.m_maxAltitude = static_cast<Altitude>(maximum);
  r.m_maxErrorMm = ReadVarUint<uint32_t>(src);
  r.m_codec = static_cast<RasterCodec>(ReadPrimitiveFromSource<uint8_t>(src));
  r.m_dataOffset = ReadVarUint<uint64_t>(src);
  r.m_dataLength = ReadVarUint<uint32_t>(src);
  return r;
}

// Owns the frozen axes and reconstructed integer heights, independently of the
// Reader/registry handle. Rows run south to north, x fastest. Cells are owned once;
// neighboring patches duplicate only perimeter nodes.
struct RasterPatch
{
  uint8_t m_coordBits = kTerrainCoordBits;
  std::vector<uint32_t> m_columns, m_rows;
  std::vector<Altitude> m_heights;

  uint32_t CellsX() const { return static_cast<uint32_t>(m_columns.size() - 1); }
  uint32_t CellsY() const { return static_cast<uint32_t>(m_rows.size() - 1); }
  m2::PointU PointU(uint32_t node) const
  {
    return {m_columns[node % m_columns.size()], m_rows[node / m_columns.size()]};
  }
  m2::PointD Point(uint32_t node) const { return PointUToPointD(PointU(node), m_coordBits); }
  m2::RectD GetRect() const { return {Point(0), Point(static_cast<uint32_t>(m_heights.size() - 1))}; }

  // CCW SW-SE-NE and SW-NE-NW, with the SW-NE diagonal fixed in every cell.
  std::array<uint32_t, 3> Triangle(uint32_t triangle) const;
  uint32_t Neighbor(uint32_t triangle, uint8_t edge) const;
};

// Perimeter indices, counterclockwise from SW, each corner exactly once.
std::vector<uint32_t> RasterPerimeter(uint32_t cellsX, uint32_t cellsY);
std::vector<Altitude> DecodeRasterPayload(RasterRecord const & record, std::span<uint8_t const> payload);
}  // namespace terrain
