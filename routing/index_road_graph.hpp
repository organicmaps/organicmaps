#pragma once

#include "routing/index_graph_starter.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/road_graph.hpp"
#include "routing/segment.hpp"

#include "indexer/index.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"

namespace routing
{
class IndexRoadGraph : public RoadGraphBase
{
public:
  IndexRoadGraph(shared_ptr<NumMwmIds> numMwmIds, IndexGraphStarter & starter,
                 vector<Segment> const & segments, vector<Junction> const & junctions,
                 Index & index);

  // IRoadGraphBase overrides:
  virtual void GetOutgoingEdges(Junction const & junction, TEdgeVector & edges) const override;
  virtual void GetIngoingEdges(Junction const & junction, TEdgeVector & edges) const override;
  virtual double GetMaxSpeedKMPH() const override;
  virtual void GetEdgeTypes(Edge const & edge, feature::TypesHolder & types) const override;
  virtual void GetJunctionTypes(Junction const & junction,
                                feature::TypesHolder & types) const override;

private:
  void GetEdges(Junction const & junction, bool isOutgoing, TEdgeVector & edges) const;
  Junction GetJunction(Segment const & segment, bool front) const;
  vector<Segment> const & GetSegments(Junction const & junction, bool isOutgoing) const;

  Index & m_index;
  shared_ptr<NumMwmIds> m_numMwmIds;
  IndexGraphStarter & m_starter;
  map<Junction, vector<Segment>> m_beginToSegment;
  map<Junction, vector<Segment>> m_endToSegment;
};
}  // namespace routing
