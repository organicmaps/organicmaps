#pragma once

#include "generator/osm_element.hpp"

#include "coding/file_writer.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"

#include "geometry/latlon.hpp"

#include <vector>

namespace generator
{
struct MiniRoundaboutInfo
{
  MiniRoundaboutInfo() = default;
  explicit MiniRoundaboutInfo(OsmElement const & element);
  MiniRoundaboutInfo(uint64_t id, ms::LatLon const & coord, std::vector<uint64_t> && ways);

  uint64_t m_id = 0;
  ms::LatLon m_coord;
  std::vector<uint64_t> m_ways;
};

std::vector<MiniRoundaboutInfo> ReadMiniRoundabouts(std::string const & filePath);

template <typename Src>
MiniRoundaboutInfo ReadMiniRoundabout(Src & src)
{
  MiniRoundaboutInfo rb;
  rb.m_id = ReadPrimitiveFromSource<uint64_t>(src);
  rb.m_coord.m_lat = ReadPrimitiveFromSource<double>(src);
  rb.m_coord.m_lon = ReadPrimitiveFromSource<double>(src);
  rw::ReadVectorOfPOD(src, rb.m_ways);
  return rb;
}

template <typename Dst>
void WriteMiniRoundabout(Dst & dst, MiniRoundaboutInfo const & rb)
{
  if (rb.m_ways.empty())
    return;
  dst.Write(&rb.m_id, sizeof(rb.m_id));
  dst.Write(&rb.m_coord.m_lat, sizeof(rb.m_coord.m_lat));
  dst.Write(&rb.m_coord.m_lon, sizeof(rb.m_coord.m_lon));
  rw::WriteVectorOfPOD(dst, rb.m_ways);
}
}  // namespace generator
