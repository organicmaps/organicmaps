#include "routing/index_road_graph.hpp"

#include "routing/data_source.hpp"
#include "routing/fake_feature_ids.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/transit_graph.hpp"

#include "coding/point_coding.hpp"

namespace routing
{
IndexRoadGraph::IndexRoadGraph(IndexGraphStarter & starter, std::vector<Segment> const & segments,
                               RouteJunctions const & junctions, MwmDataSource & dataSource)
  : m_dataSource(dataSource)
  , m_starter(starter)
  , m_segments(segments)
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

void IndexRoadGraph::GetOutgoingEdges(geometry::PointWithAltitude const & junction, EdgeListT & edges) const
{
  GetEdges(junction, true, edges);
}

void IndexRoadGraph::GetIngoingEdges(geometry::PointWithAltitude const & junction, EdgeListT & edges) const
{
  GetEdges(junction, false, edges);
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

  auto ft = m_dataSource.GetFeature(featureId);
  if (!ft)
  {
    LOG(LERROR, ("Can't load types for feature", featureId));
    return;
  }

  ASSERT_EQUAL(ft->GetGeomType(), feature::GeomType::Line, ());
  types = feature::TypesHolder(*ft);
}

void IndexRoadGraph::GetJunctionTypes(geometry::PointWithAltitude const & junction, feature::TypesHolder & types) const
{
  types = feature::TypesHolder();
}

void IndexRoadGraph::GetRouteEdges(EdgeVector & edges) const
{
  edges.clear();
  edges.reserve(m_segments.size());

  for (Segment const & segment : m_segments)
  {
    auto const & junctionFrom = m_starter.GetJunction(segment, false /* front */).ToPointWithAltitude();
    auto const & junctionTo = m_starter.GetJunction(segment, true /* front */).ToPointWithAltitude();

    if (IndexGraphStarter::IsFakeSegment(segment) || TransitGraph::IsTransitSegment(segment))
    {
      Segment real = segment;
      if (m_starter.ConvertToReal(real))
      {
        edges.push_back(Edge::MakeFakeWithRealPart({m_dataSource.GetMwmId(real.GetMwmId()), real.GetFeatureId()},
                                                   segment.GetSegmentIdx(), real.IsForward(), real.GetSegmentIdx(),
                                                   junctionFrom, junctionTo));
      }
      else
      {
        edges.push_back(Edge::MakeFake(junctionFrom, junctionTo));
      }
    }
    else
    {
      edges.push_back(Edge::MakeReal({m_dataSource.GetMwmId(segment.GetMwmId()), segment.GetFeatureId()},
                                     segment.IsForward(), segment.GetSegmentIdx(), junctionFrom, junctionTo));
    }
  }
}

void IndexRoadGraph::GetEdges(geometry::PointWithAltitude const & junction, bool isOutgoing, EdgeListT & edges) const
{
  edges.clear();

  // Features crossing mwm borders are duplicated in both mwms, so the same physical edge
  // can be returned twice: as an edge of the current mwm and as its twin from the neighbouring one.
  // Such duplicates should be filtered out, otherwise turns generation sees a phantom parallel road
  // and produces dummy GoStraight maneuvers near mwm borders. See
  // https://github.com/organicmaps/organicmaps/issues/5804
  auto const isTwinDuplicate = [&edges](Edge const & edge)
  {
    for (Edge const & e : edges)
      if (e.GetFeatureId().m_mwmId != edge.GetFeatureId().m_mwmId &&
          m2::AlmostEqualAbs(e.GetStartPoint(), edge.GetStartPoint(), kMwmPointAccuracy) &&
          m2::AlmostEqualAbs(e.GetEndPoint(), edge.GetEndPoint(), kMwmPointAccuracy))
        return true;
    return false;
  };

  for (Segment const & segment : GetSegments(junction, isOutgoing))
  {
    IndexGraphStarter::EdgeListT tmpEdges;
    m_starter.GetEdgesList(segment, isOutgoing, tmpEdges);

    for (SegmentEdge const & segmentEdge : tmpEdges)
    {
      Segment const & segment = segmentEdge.GetTarget();
      if (IndexGraphStarter::IsFakeSegment(segment))
        continue;

      Edge edge = Edge::MakeReal({m_dataSource.GetMwmId(segment.GetMwmId()), segment.GetFeatureId()},
                                 segment.IsForward(), segment.GetSegmentIdx(),
                                 m_starter.GetJunction(segment, false /* front */).ToPointWithAltitude(),
                                 m_starter.GetJunction(segment, true /* front */).ToPointWithAltitude());

      // Edges of the current mwm are returned first (twins are appended after them),
      // so the same-mwm copy of a duplicated edge is kept.
      if (!isTwinDuplicate(edge))
        edges.push_back(std::move(edge));
    }
  }
}

IndexRoadGraph::SegmentListT const & IndexRoadGraph::GetSegments(geometry::PointWithAltitude const & junction,
                                                                 bool isOutgoing) const
{
  auto const & junctionToSegment = isOutgoing ? m_endToSegment : m_beginToSegment;

  auto const it = junctionToSegment.find(junction);
  CHECK(it != junctionToSegment.cend(), ("junctionToSegment doesn't contain", junction, ", isOutgoing =", isOutgoing));
  return it->second;
}
}  // namespace routing
