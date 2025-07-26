#pragma once

#include "generator/osm_element.hpp"

#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/latlon.hpp"

#include <vector>

namespace feature
{
class FeatureBuilder;
}

namespace generator
{
struct MiniRoundaboutInfo
{
  MiniRoundaboutInfo() = default;
  explicit MiniRoundaboutInfo(OsmElement const & element);

  bool Normalize();

  static bool IsProcessRoad(feature::FeatureBuilder const & fb);

  uint64_t m_id = 0;
  ms::LatLon m_coord;
  std::vector<uint64_t> m_ways;
};

class MiniRoundaboutData
{
public:
  MiniRoundaboutData(std::vector<MiniRoundaboutInfo> && data);

  bool IsRoadExists(feature::FeatureBuilder const & fb) const;
  std::vector<MiniRoundaboutInfo> const & GetData() const { return m_data; }

private:
  std::vector<MiniRoundaboutInfo> m_data;
  std::vector<uint64_t> m_ways;
};

MiniRoundaboutData ReadMiniRoundabouts(std::string const & filePath);

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
  WriteToSink(dst, rb.m_id);
  dst.Write(&rb.m_coord.m_lat, sizeof(rb.m_coord.m_lat));
  dst.Write(&rb.m_coord.m_lon, sizeof(rb.m_coord.m_lon));
  rw::WriteVectorOfPOD(dst, rb.m_ways);
}
}  // namespace generator
