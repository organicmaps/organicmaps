#include "generator/mini_roundabout_info.hpp"

#include "generator/feature_builder.hpp"

#include "routing/routing_helpers.hpp"

#include "coding/file_reader.hpp"

#include <algorithm>

namespace generator
{
MiniRoundaboutInfo::MiniRoundaboutInfo(OsmElement const & element)
  : m_id(element.m_id)
  , m_coord(element.m_lat, element.m_lon)
{}

bool MiniRoundaboutInfo::Normalize()
{
  if (m_ways.empty())
    return false;

  std::sort(std::begin(m_ways), std::end(m_ways));
  return true;
}

bool MiniRoundaboutInfo::IsProcessRoad(feature::FeatureBuilder const & fb)
{
  // Important! Collect and transform only Car roundabouts.
  return fb.IsLine() && routing::IsCarRoad(fb.GetTypes());
}

MiniRoundaboutData::MiniRoundaboutData(std::vector<MiniRoundaboutInfo> && data) : m_data(std::move(data))
{
  for (auto const & d : m_data)
    m_ways.insert(std::end(m_ways), std::cbegin(d.m_ways), std::cend(d.m_ways));

  base::SortUnique(m_ways);
}

bool MiniRoundaboutData::IsRoadExists(feature::FeatureBuilder const & fb) const
{
  if (!MiniRoundaboutInfo::IsProcessRoad(fb))
    return false;

  return std::binary_search(std::cbegin(m_ways), std::cend(m_ways), fb.GetMostGenericOsmId().GetSerialId());
}

MiniRoundaboutData ReadMiniRoundabouts(std::string const & filePath)
{
  FileReader reader(filePath);
  ReaderSource<FileReader> src(reader);
  std::vector<MiniRoundaboutInfo> res;

  while (src.Size() > 0)
    res.push_back(ReadMiniRoundabout(src));

  LOG(LINFO, ("Loaded", res.size(), "mini_roundabouts from file", filePath));
  return res;
}
}  // namespace generator
