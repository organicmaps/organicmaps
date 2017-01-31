#include "routing/index_road_graph.hpp"

namespace routing
{
IndexRoadGraph::IndexRoadGraph(MwmSet::MwmId const & mwmId, Index const & index,
                               double maxSpeedKMPH, IndexGraphStarter & starter,
                               vector<Segment> const & segments, vector<Junction> const & junctions)
  : m_mwmId(mwmId), m_index(index), m_maxSpeedKMPH(maxSpeedKMPH), m_starter(starter)
{
  CHECK_EQUAL(segments.size(), junctions.size() + 1, ());

  for (size_t i = 0; i < junctions.size(); ++i)
  {
    Junction const & junction = junctions[i];
    m_endToSegment[junction].push_back(segments[i]);
    m_beginToSegment[junction].push_back(segments[i + 1]);
  }
}

void IndexRoadGraph::GetOutgoingEdges(Junction const & junction, TEdgeVector & edges) const
{
  GetEdges(junction, true, edges);
}

void IndexRoadGraph::GetIngoingEdges(Junction const & junction, TEdgeVector & edges) const
{
  GetEdges(junction, false, edges);
}

double IndexRoadGraph::GetMaxSpeedKMPH() const
{
  return m_maxSpeedKMPH;
}

void IndexRoadGraph::GetEdgeTypes(Edge const & edge, feature::TypesHolder & types) const
{
  if (edge.IsFake())
  {
    types = feature::TypesHolder(feature::GEOM_LINE);
    return;
  }

  FeatureID const featureId = edge.GetFeatureId();
  FeatureType ft;
  Index::FeaturesLoaderGuard loader(m_index, featureId.m_mwmId);
  if (!loader.GetFeatureByIndex(featureId.m_index, ft))
  {
    LOG(LERROR, ("Can't load types for feature", featureId));
    return;
  }

  ASSERT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, ());
  types = feature::TypesHolder(ft);
}

void IndexRoadGraph::GetJunctionTypes(Junction const & junction, feature::TypesHolder & types) const
{
  // TODO: implement method to support PedestrianDirection::LiftGate, PedestrianDirection::Gate
  types = feature::TypesHolder();
}

void IndexRoadGraph::GetEdges(Junction const & junction, bool isOutgoing, TEdgeVector & edges) const
{
  edges.clear();

  vector<SegmentEdge> segmentEdges;
  vector<SegmentEdge> tmpEdges;
  for (Segment const & segment : GetSegments(junction, isOutgoing))
  {
    tmpEdges.clear();
    m_starter.GetEdgesList(segment, isOutgoing, tmpEdges);
    segmentEdges.insert(segmentEdges.end(), tmpEdges.begin(), tmpEdges.end());
  }

  for (SegmentEdge const & segmentEdge : segmentEdges)
  {
    Segment const & segment = segmentEdge.GetTarget();
    if (IndexGraphStarter::IsFakeSegment(segment))
      continue;

    edges.emplace_back(FeatureID(m_mwmId, segment.GetFeatureId()), segment.IsForward(),
                       segment.GetSegmentIdx(), GetJunction(segment, false /* front */),
                       GetJunction(segment, true /* front */));
  }
}

Junction IndexRoadGraph::GetJunction(Segment const & segment, bool front) const
{
  // TODO: Use real altitudes for pedestrian and bicycle routing.
  return Junction(m_starter.GetPoint(segment, front), feature::kDefaultAltitudeMeters);
}

vector<Segment> const & IndexRoadGraph::GetSegments(Junction const & junction,
                                                    bool isOutgoing) const
{
  auto const & junctionToSegment = isOutgoing ? m_endToSegment : m_beginToSegment;

  auto const it = junctionToSegment.find(junction);
  CHECK(it != junctionToSegment.cend(),
        ("junctionToSegment doesn't contain", junction, ", isOutgoing =", isOutgoing));
  return it->second;
}
}  // namespace routing
