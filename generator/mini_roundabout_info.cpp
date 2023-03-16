#include "generator/mini_roundabout_info.hpp"

#include "coding/file_reader.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace generator
{
MiniRoundaboutInfo::MiniRoundaboutInfo(OsmElement const & element)
  : m_id(element.m_id), m_coord(element.m_lat, element.m_lon)
{
}

MiniRoundaboutInfo::MiniRoundaboutInfo(uint64_t id, ms::LatLon const & coord,
                                       std::vector<uint64_t> && ways)
  : m_id(id), m_coord(coord), m_ways(std::move(ways))
{
}

void MiniRoundaboutInfo::Normalize() { std::sort(std::begin(m_ways), std::end(m_ways)); }

std::vector<MiniRoundaboutInfo> ReadMiniRoundabouts(std::string const & filePath)
{
  FileReader reader(filePath);
  ReaderSource<FileReader> src(reader);
  std::vector<MiniRoundaboutInfo> res;

  while (src.Size() > 0)
    res.push_back(ReadMiniRoundabout(src));

  return res;
}
}  // namespace generator
