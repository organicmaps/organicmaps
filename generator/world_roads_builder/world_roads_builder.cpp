#include "generator/world_roads_builder/world_roads_builder.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <string>

namespace
{
using namespace routing;

size_t constexpr kMaxRoadsForRegion = 10;
std::string const kDelim = " ";

// Returns true if roads count for |mwmId| in |graph| exceeds max value.
bool MwmRoadsAreFilled(routing::NumMwmId const & mwmId, CrossBorderGraph & graph)
{
  auto const it = graph.m_mwms.find(mwmId);
  if (it == graph.m_mwms.end())
    return false;

  return it->second.size() >= kMaxRoadsForRegion;
}

void WriteEndingToSteam(CrossBorderSegmentEnding const & segEnding, std::ofstream & output)
{
  auto const & point = segEnding.m_point.GetLatLon();
  output << kDelim << point.m_lat << kDelim << point.m_lon << kDelim << segEnding.m_numMwmId;
}

void WriteSegmentToStream(RegionSegmentId const & segId, CrossBorderSegment const & seg,
                          std::ofstream & output)
{
  CHECK(output.is_open(), ());

  output << segId << kDelim << static_cast<size_t>(std::ceil(seg.m_weight));
  WriteEndingToSteam(seg.m_start, output);
  WriteEndingToSteam(seg.m_end, output);
  output << std::endl;
}
}  // namespace

namespace routing
{
RoadsFromOsm GetRoadsFromOsm(generator::SourceReader & reader,
                             feature::CountriesFilesAffiliation const & mwmMatcher,
                             std::vector<std::string> const & highways)
{
  RoadsFromOsm roadsFromOsm;

  ProcessOsmElementsFromO5M(reader, [&roadsFromOsm, &highways](OsmElement * e) {
    if (e->IsWay())
    {
      std::string const & highway = e->GetTag("highway");

      if (!highway.empty() &&
          std::find(highways.begin(), highways.end(), highway) != highways.end())
      {
        roadsFromOsm.m_ways[highway].emplace(e->m_id, RoadData({}, *e));
      }
    }
    else if (e->IsNode())
    {
      roadsFromOsm.m_nodes.emplace(e->m_id, ms::LatLon(e->m_lat, e->m_lon));
    }
  });

  LOG(LINFO, ("Extracted", roadsFromOsm.m_nodes.size(), "nodes from OSM."));

  for (auto & [highway, ways] : roadsFromOsm.m_ways)
  {
    LOG(LINFO, ("Extracted", ways.size(), highway, "highways from OSM."));

    for (auto & [wayId, wayData] : ways)
    {
      for (auto const & nodeId : {wayData.m_way.Nodes().front(), wayData.m_way.Nodes().back()})
      {
        auto const nodeIt = roadsFromOsm.m_nodes.find(nodeId);
        CHECK(nodeIt != roadsFromOsm.m_nodes.end(), (nodeId));

        m2::PointD const & point = mercator::FromLatLon(nodeIt->second);
        auto const & regions = mwmMatcher.GetAffiliations(point);

        for (auto const & region : regions)
          wayData.m_regions.emplace(region);
      }
    }
  }

  LOG(LINFO, ("Filled regions for ways."));

  return roadsFromOsm;
}

void FillCrossBorderGraph(CrossBorderGraph & graph, RegionSegmentId & curSegmentId,
                          std::vector<uint64_t> const & nodeIds,
                          std::unordered_map<uint64_t, ms::LatLon> const & nodes,
                          feature::CountriesFilesAffiliation const & mwmMatcher,
                          std::unordered_map<std::string, NumMwmId> const & regionToIdMap)
{
  std::string prevRegion;
  m2::PointD prevPoint;

  for (auto const & nodeId : nodeIds)
  {
    auto const itNodes = nodes.find(nodeId);
    CHECK(itNodes != nodes.end(), (nodeId));

    m2::PointD const & curPoint = mercator::FromLatLon(itNodes->second);
    auto const & regions = mwmMatcher.GetAffiliations(curPoint);

    if (regions.size() > 1)
    {
      LOG(LWARNING, ("Point", itNodes->second, "belongs to multiple mwms:", regions));
      continue;
    }

    if (regions.empty())
    {
      LOG(LWARNING, ("Point", itNodes->second, "doesn't belong to any mwm."));
      continue;
    }

    if (auto const & curRegion = regions[0]; curRegion != prevRegion)
    {
      if (!prevRegion.empty())
      {
        auto const & prevMwmId = regionToIdMap.at(prevRegion);
        auto const & curMwmId = regionToIdMap.at(curRegion);

        if (MwmRoadsAreFilled(prevMwmId, graph) && MwmRoadsAreFilled(curMwmId, graph))
          continue;

        CrossBorderSegment seg;
        seg.m_weight = mercator::DistanceOnEarth(prevPoint, curPoint);
        seg.m_start = CrossBorderSegmentEnding(prevPoint, prevMwmId);
        seg.m_end = CrossBorderSegmentEnding(curPoint, curMwmId);

        graph.AddCrossBorderSegment(curSegmentId++, seg);
      }

      prevRegion = curRegion;
    }

    prevPoint = curPoint;
  }
}

bool WriteGraphToFile(CrossBorderGraph const & graph, std::string const & path, bool overwrite)
{
  std::ofstream output;
  output.exceptions(std::ofstream::failbit | std::ofstream::badbit);

  try
  {
    std::ios_base::openmode mode = overwrite ? std::ofstream::trunc : std::ofstream::app;
    output.open(path, mode);

    for (auto const & [segId, segData] : graph.m_segments)
      WriteSegmentToStream(segId, segData, output);
  }
  catch (std::ofstream::failure const & se)
  {
    LOG(LWARNING, ("Exception saving roads to file", path, se.what()));
    return false;
  }

  return true;
}

void ShowRegionsStats(CrossBorderGraph const & graph, std::shared_ptr<routing::NumMwmIds> numMwmIds)
{
  for (auto const & [mwmId, segmentIds] : graph.m_mwms)
    LOG(LINFO, (numMwmIds->GetFile(mwmId).GetName(), "roads:", segmentIds.size()));
}
}  // namespace routing
