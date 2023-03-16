#pragma once

#include "generator/osm_element.hpp"

#include "coding/file_writer.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/latlon.hpp"

#include <tuple>
#include <vector>

namespace generator
{
struct MiniRoundaboutInfo
{
  MiniRoundaboutInfo() = default;
  explicit MiniRoundaboutInfo(OsmElement const & element);
  explicit MiniRoundaboutInfo(uint64_t id, ms::LatLon const & coord, std::vector<uint64_t> && ways);

  friend bool operator<(MiniRoundaboutInfo const & lhs, MiniRoundaboutInfo const & rhs)
  {
    return std::tie(lhs.m_id, lhs.m_coord, lhs.m_ways) <
           std::tie(rhs.m_id, rhs.m_coord, rhs.m_ways);
  }

  void Normalize();

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
  src.Read(&rb.m_coord.m_lat, sizeof(rb.m_coord.m_lat));
  src.Read(&rb.m_coord.m_lon, sizeof(rb.m_coord.m_lon));
  rw::ReadVectorOfPOD(src, rb.m_ways);
  return rb;
}

template <typename Dst>
void WriteMiniRoundabout(Dst & dst, MiniRoundaboutInfo const & rb)
{
  if (rb.m_ways.empty())
    return;
  WriteToSink(dst, rb.m_id);
  dst.Write(&rb.m_coord.m_lat, sizeof(rb.m_coord.m_lat));
  dst.Write(&rb.m_coord.m_lon, sizeof(rb.m_coord.m_lon));
  rw::WriteVectorOfPOD(dst, rb.m_ways);
}
}  // namespace generator
