#include "routing/index_road_graph.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/transit_graph.hpp"

#include "indexer/data_source.hpp"

#include <cstdint>
#include <utility>

using namespace std;

namespace routing
{
IndexRoadGraph::IndexRoadGraph(shared_ptr<NumMwmIds> numMwmIds, IndexGraphStarter & starter,
                               vector<Segment> const & segments,
                               vector<geometry::PointWithAltitude> const & junctions,
                               DataSource & dataSource)
  : m_dataSource(dataSource), m_numMwmIds(move(numMwmIds)), m_starter(starter), m_segments(segments)
{
  //    j0     j1     j2     j3
  //    *--s0--*--s1--*--s2--*
  CHECK_EQUAL(segments.size() + 1, junctions.size(), ());

  for (size_t i = 0; i < junctions.size(); ++i)
  {
    geometry::PointWithAltitude const & junction = junctions[i];
    if (i > 0)
      m_endToSegment[junction].push_back(segments[i - 1]);
    if (i < segments.size())
      m_beginToSegment[junction].push_back(segments[i]);
  }
}

void IndexRoadGraph::GetOutgoingEdges(geometry::PointWithAltitude const & junction,
                                      EdgeVector & edges) const
{
  GetEdges(junction, true, edges);
}

void IndexRoadGraph::GetIngoingEdges(geometry::PointWithAltitude const & junction,
                                     EdgeVector & edges) const
{
  GetEdges(junction, false, edges);
}

double IndexRoadGraph::GetMaxSpeedKMpH() const
{
  // Value doesn't matter.
  // It is used in CalculateMaxSpeedTimes only.
  // Then SingleMwmRouter::RedressRoute overwrites time values.
  //
  // TODO: remove this stub after transfering Bicycle and Pedestrian to index routing.
  return 0.0;
}

void IndexRoadGraph::GetEdgeTypes(Edge const & edge, feature::TypesHolder & types) const
{
  if (edge.IsFake())
  {
    types = feature::TypesHolder(feature::GeomType::Line);
    return;
  }

  FeatureID const featureId = edge.GetFeatureId();
  if (FakeFeatureIds::IsGuidesFeature(featureId.m_index))
  {
    types = feature::TypesHolder(feature::GeomType::Line);
    return;
  }

  FeaturesLoaderGuard loader(m_dataSource, featureId.m_mwmId);
  auto ft = loader.GetFeatureByIndex(featureId.m_index);
  if (!ft)
  {
    LOG(LERROR, ("Can't load types for feature", featureId));
    return;
  }

  ASSERT_EQUAL(ft->GetGeomType(), feature::GeomType::Line, ());
  types = feature::TypesHolder(*ft);
}

void IndexRoadGraph::GetJunctionTypes(geometry::PointWithAltitude const & junction,
                                      feature::TypesHolder & types) const
{
  types = feature::TypesHolder();
}

void IndexRoadGraph::GetRouteEdges(EdgeVector & edges) const
{
  edges.clear();
  edges.reserve(m_segments.size());

  for (Segment const & segment : m_segments)
  {
    auto const & junctionFrom =
        m_starter.GetJunction(segment, false /* front */).ToPointWithAltitude();
    auto const & junctionTo =
        m_starter.GetJunction(segment, true /* front */).ToPointWithAltitude();

    if (IndexGraphStarter::IsFakeSegment(segment) || TransitGraph::IsTransitSegment(segment))
    {
      Segment real = segment;
      if (m_starter.ConvertToReal(real))
      {
        platform::CountryFile const & file = m_numMwmIds->GetFile(real.GetMwmId());
        MwmSet::MwmId const mwmId = m_dataSource.GetMwmIdByCountryFile(file);
        edges.push_back(Edge::MakeFakeWithRealPart(FeatureID(mwmId, real.GetFeatureId()),
                                                   segment.GetSegmentIdx(),
                                                   real.IsForward(), real.GetSegmentIdx(),
                                                   junctionFrom, junctionTo));
      }
      else
      {
        edges.push_back(Edge::MakeFake(junctionFrom, junctionTo));
      }
    }
    else
    {
      platform::CountryFile const & file = m_numMwmIds->GetFile(segment.GetMwmId());
      MwmSet::MwmId const mwmId = m_dataSource.GetMwmIdByCountryFile(file);
      edges.push_back(Edge::MakeReal(FeatureID(mwmId, segment.GetFeatureId()), segment.IsForward(),
                                     segment.GetSegmentIdx(), junctionFrom, junctionTo));
    }
  }
}

void IndexRoadGraph::GetRouteSegments(std::vector<Segment> & segments) const
{
  segments = m_segments;
}

void IndexRoadGraph::GetEdges(geometry::PointWithAltitude const & junction, bool isOutgoing,
                              EdgeVector & edges) const
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

    platform::CountryFile const & file = m_numMwmIds->GetFile(segment.GetMwmId());
    MwmSet::MwmId const mwmId = m_dataSource.GetMwmIdByCountryFile(file);

    edges.push_back(Edge::MakeReal(
        FeatureID(mwmId, segment.GetFeatureId()), segment.IsForward(), segment.GetSegmentIdx(),
        m_starter.GetJunction(segment, false /* front */).ToPointWithAltitude(),
        m_starter.GetJunction(segment, true /* front */).ToPointWithAltitude()));
  }
}

vector<Segment> const & IndexRoadGraph::GetSegments(geometry::PointWithAltitude const & junction,
                                                    bool isOutgoing) const
{
  auto const & junctionToSegment = isOutgoing ? m_endToSegment : m_beginToSegment;

  auto const it = junctionToSegment.find(junction);
  CHECK(it != junctionToSegment.cend(),
        ("junctionToSegment doesn't contain", junction, ", isOutgoing =", isOutgoing));
  return it->second;
}
}  // namespace routing
