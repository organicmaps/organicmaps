#include "routing/regions_sparse_graph.hpp"

#include "indexer/utils.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"

#include "coding/file_reader.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"

namespace routing
{
RegionsSparseGraph::RegionsSparseGraph(CountryFileGetterFn const & countryFileGetter,
                                       std::shared_ptr<NumMwmIds> numMwmIds,
                                       DataSource & dataSource)
  : m_countryFileGetterFn(countryFileGetter), m_numMwmIds(numMwmIds), m_dataSource(dataSource)
{
}

void RegionsSparseGraph::LoadRegionsSparseGraph()
{
  MwmSet::MwmHandle mwmWorld = indexer::FindWorld(m_dataSource);
  if (!mwmWorld.IsAlive())
  {
    LOG(LWARNING, ("Couldn't open world mwm."));
    return;
  }

  auto const & mwmValue = *mwmWorld.GetValue();
  if (!mwmValue.m_cont.IsExist(ROUTING_WORLD_FILE_TAG))
  {
    LOG(LWARNING, ("Section", ROUTING_WORLD_FILE_TAG, "doesn't exist in world mwm."));
    return;
  }

  ReaderSource<FilesContainerR::TReader> reader(mwmValue.m_cont.GetReader(ROUTING_WORLD_FILE_TAG));
  CrossBorderGraphSerializer::Deserialize(m_graph, reader, m_numMwmIds);
}

std::optional<FakeEnding> RegionsSparseGraph::GetFakeEnding(m2::PointD const & point) const
{
  NumMwmId const mwmId = m_numMwmIds->GetId(platform::CountryFile(m_countryFileGetterFn(point)));
  auto const it = m_graph.m_mwms.find(mwmId);
  if (it == m_graph.m_mwms.end())
    return std::nullopt;

  FakeEnding ending;

  ending.m_originJunction = LatLonWithAltitude(mercator::ToLatLon(point), 0);

  for (auto const & segmentId : it->second)
  {
    CrossBorderSegment const & data = GetDataById(segmentId);

    bool const oneWay = false;
    auto const & frontJunction = data.m_end.m_point;
    auto const & backJunction = data.m_start.m_point;
    auto const & projectedJunction = CalcProjectionToSegment(backJunction, frontJunction, point);

    Segment const segment(data.m_start.m_numMwmId, segmentId /* featureId */, 0 /* segmentIdx */,
                          true /* isForward */);

    ending.m_projections.emplace_back(segment, oneWay, frontJunction, backJunction,
                                      projectedJunction);
  }

  return ending;
}

void RegionsSparseGraph::GetEdgeList(Segment const & segment, bool isOutgoing,
                                     std::vector<SegmentEdge> & edges,
                                     RouteWeight const & prevWeight) const
{
  auto const & data = GetDataById(segment.GetFeatureId());

  NumMwmId const targetMwm =
      (segment.IsForward() == isOutgoing) ? data.m_end.m_numMwmId : data.m_start.m_numMwmId;

  auto it = m_graph.m_mwms.find(targetMwm);
  if (it == m_graph.m_mwms.end())
    return;

  for (auto const & id : it->second)
  {
    auto const outData = GetDataById(id);
    auto const weight = isOutgoing ? RouteWeight(outData.m_weight) : prevWeight;

    Segment const edge(outData.m_start.m_numMwmId, id /* featureId */, 0 /* segmentIdx */,
                       segment.IsForward() /* isForward */);
    edges.emplace_back(edge, weight);
  }
}

routing::LatLonWithAltitude const & RegionsSparseGraph::GetJunction(Segment const & segment,
                                                                    bool front) const
{
  auto const & data = GetDataById(segment.GetFeatureId());
  return segment.IsForward() == front ? data.m_end.m_point : data.m_start.m_point;
}

RouteWeight RegionsSparseGraph::CalcSegmentWeight(Segment const & segment) const
{
  return RouteWeight(GetDataById(segment.GetFeatureId()).m_weight);
}

CrossBorderSegment const & RegionsSparseGraph::GetDataById(RegionSegmentId const & id) const
{
  auto it = m_graph.m_segments.find(id);
  CHECK(it != m_graph.m_segments.end(), (id));
  return it->second;
}
}  // namespace routing
