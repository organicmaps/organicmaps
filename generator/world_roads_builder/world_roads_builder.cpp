#include "generator/world_roads_builder/world_roads_builder.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <string>

namespace
{
using namespace routing;

size_t constexpr kMaxRoadsCount = 5;
std::string const kDelim = " ";
double constexpr kMinSegmentLengthM = 200.0;
double constexpr kHalfSegmentLengthM = kMinSegmentLengthM / 2.0;

// Returns true if roads count between |mwmId1| and |mwmId2| in |graph| exceeds max value.
bool MwmRoadsAreFilled(routing::NumMwmId const & mwmId1, routing::NumMwmId const & mwmId2, CrossBorderGraph & graph)
{
  auto const it = graph.m_mwms.find(mwmId1);
  if (it == graph.m_mwms.end())
    return false;

  size_t count = 0;

  for (auto segId : it->second)
  {
    auto const segDataIt = graph.m_segments.find(segId);
    CHECK(segDataIt != graph.m_segments.end(), (segId));
    auto const & segData = segDataIt->second;

    if (segData.m_start.m_numMwmId == mwmId2 || segData.m_end.m_numMwmId == mwmId2)
      ++count;
  }

  return count >= kMaxRoadsCount;
}

void WriteEndingToSteam(CrossBorderSegmentEnding const & segEnding, std::ofstream & output)
{
  auto const & point = segEnding.m_point.GetLatLon();
  output << kDelim << point.m_lat << kDelim << point.m_lon << kDelim << segEnding.m_numMwmId;
}

void WriteSegmentToStream(RegionSegmentId const & segId, CrossBorderSegment const & seg, std::ofstream & output)
{
  CHECK(output.is_open(), ());

  output << segId << kDelim << seg.m_weight;
  WriteEndingToSteam(seg.m_start, output);
  WriteEndingToSteam(seg.m_end, output);
  output << std::endl;
}
}  // namespace

namespace routing
{
RoadsFromOsm GetRoadsFromOsm(generator::SourceReader & reader, feature::CountriesFilesAffiliation const & mwmMatcher,
                             std::vector<std::string> const & highways)
{
  RoadsFromOsm roadsFromOsm;

  ProcessOsmElementsFromO5M(reader, [&roadsFromOsm, &highways](OsmElement && e)
  {
    if (e.IsWay())
    {
      std::string const & highway = e.GetTag("highway");

      if (!highway.empty() && base::IsExist(highways, highway))
      {
        auto id = e.m_id;
        roadsFromOsm.m_ways[highway].emplace(id, RoadData({}, std::move(e)));
      }
    }
    else if (e.IsNode())
    {
      roadsFromOsm.m_nodes.emplace(e.m_id, ms::LatLon(e.m_lat, e.m_lon));
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

struct NodePoint
{
  NodePoint(m2::PointD const & point, NumMwmId const & mwm) : m_point(point), m_mwm(mwm) {}

  m2::PointD m_point;
  NumMwmId m_mwm = 0;
};

using NodePoints = std::vector<NodePoint>;
using CrossBorderIndexes = std::vector<size_t>;

std::pair<NodePoints, CrossBorderIndexes> GetCrossBorderPoints(
    std::vector<uint64_t> const & nodeIds, std::unordered_map<uint64_t, ms::LatLon> const & nodes,
    feature::CountriesFilesAffiliation const & mwmMatcher,
    std::unordered_map<std::string, NumMwmId> const & regionToIdMap)
{
  NodePoints nodePoints;
  CrossBorderIndexes crossBorderIndexes;

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

    auto const & curRegion = regions[0];
    auto const & curMwmId = regionToIdMap.at(curRegion);
    nodePoints.emplace_back(curPoint, curMwmId);

    if (curRegion != prevRegion)
    {
      if (!prevRegion.empty())
      {
        CHECK_GREATER(nodePoints.size(), 1, ());
        // We add index of the previous point.
        crossBorderIndexes.push_back(nodePoints.size() - 2);
      }

      prevRegion = curRegion;
    }

    prevPoint = curPoint;
  }

  return std::make_pair(nodePoints, crossBorderIndexes);
}

std::optional<std::pair<m2::PointD, double>> GetPointInMwm(NodePoints const & points, size_t index, bool forward)
{
  auto const & pointOnBorder = points[index];

  m2::PointD newPoint = pointOnBorder.m_point;
  double dist = 0.0;

  while ((!forward && index > 0) || (forward && index < points.size() - 1))
  {
    if (forward)
      ++index;
    else
      --index;

    auto const & point = points[index];

    if (point.m_mwm != pointOnBorder.m_mwm)
      break;

    double const curDist = mercator::DistanceOnEarth(pointOnBorder.m_point, point.m_point);

    if (curDist >= kHalfSegmentLengthM)
      return std::make_pair(point.m_point, curDist);

    newPoint = point.m_point;
    dist = curDist;
  }

  return std::make_pair(newPoint, dist);
}

bool FillCrossBorderGraph(CrossBorderGraph & graph, RegionSegmentId & curSegmentId,
                          std::vector<uint64_t> const & nodeIds, std::unordered_map<uint64_t, ms::LatLon> const & nodes,
                          feature::CountriesFilesAffiliation const & mwmMatcher,
                          std::unordered_map<std::string, NumMwmId> const & regionToIdMap)
{
  auto const & [nodePoints, crossBorderIndexes] = GetCrossBorderPoints(nodeIds, nodes, mwmMatcher, regionToIdMap);

  bool insertedRoad = false;

  for (auto i : crossBorderIndexes)
  {
    auto const & p1 = nodePoints[i];
    auto const & p2 = nodePoints[i + 1];

    if (MwmRoadsAreFilled(p1.m_mwm, p2.m_mwm, graph))
      continue;

    auto const pMwm1 = GetPointInMwm(nodePoints, i, false /* forward */);
    if (!pMwm1)
      continue;

    auto const pMwm2 = GetPointInMwm(nodePoints, i + 1, true /* forward */);
    if (!pMwm2)
      continue;

    double const d = mercator::DistanceOnEarth(p1.m_point, p2.m_point);

    CrossBorderSegment seg;
    seg.m_weight = pMwm1->second + d + pMwm2->second;
    seg.m_start = CrossBorderSegmentEnding(pMwm1->first /* point */, p1.m_mwm);
    seg.m_end = CrossBorderSegmentEnding(pMwm2->first /* point */, p2.m_mwm);
    graph.AddCrossBorderSegment(curSegmentId++, seg);
    insertedRoad = true;
  }

  return insertedRoad;
}

bool WriteGraphToFile(CrossBorderGraph const & graph, std::string const & path, bool overwrite)
{
  std::ofstream output;
  output.exceptions(std::ofstream::failbit | std::ofstream::badbit);

  try
  {
    std::ios_base::openmode mode = overwrite ? std::ofstream::trunc : std::ofstream::app;
    output.open(path, mode);

    output << std::setprecision(12);
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

  std::string const delimRegions = " - ";

  std::map<std::string, size_t> statsByMwm;

  for (auto const & [segId, segData] : graph.m_segments)
  {
    std::string reg1 = numMwmIds->GetFile(segData.m_start.m_numMwmId).GetName();
    std::string reg2 = numMwmIds->GetFile(segData.m_end.m_numMwmId).GetName();

    std::string k = reg1 < reg2 ? reg1 + delimRegions + reg2 : reg2 + delimRegions + reg1;
    ++statsByMwm[k];
  }

  LOG(LINFO, ("Count of roads between mwm pairs:"));

  for (auto const & [k, count] : statsByMwm)
    LOG(LINFO, (k, " -> ", count));
}
}  // namespace routing
