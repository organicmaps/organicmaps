#pragma once

#include "routing/cross_mwm_connector.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/segment.hpp"
#include "routing/transition_points.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/vehicle_model.hpp"

#include "coding/file_container.hpp"
#include "coding/reader.hpp"

#include "indexer/index.hpp"

#include <map>
#include <memory>
#include <vector>

namespace routing
{
class CrossMwmIndexGraph final
{
public:
  CrossMwmIndexGraph(Index & index, std::shared_ptr<NumMwmIds> numMwmIds)
    : m_index(index), m_numMwmIds(numMwmIds)
  {
  }

  bool IsTransition(Segment const & s, bool isOutgoing);
  void GetEdgeList(Segment const & s, bool isOutgoing, std::vector<SegmentEdge> & edges);
  void Clear() { m_connectors.clear(); }
  TransitionPoints GetTransitionPoints(Segment const & s, bool isOutgoing);
  bool HasCache(NumMwmId numMwmId) const { return m_connectors.count(numMwmId) != 0; }
private:
  CrossMwmConnector const & GetCrossMwmConnectorWithTransitions(NumMwmId numMwmId);
  CrossMwmConnector const & GetCrossMwmConnectorWithWeights(NumMwmId numMwmId);

  template <typename Fn>
  CrossMwmConnector const & Deserialize(NumMwmId numMwmId, Fn && fn)
  {
    MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(m_numMwmIds->GetFile(numMwmId));
    CHECK(handle.IsAlive(), ());
    MwmValue * value = handle.GetValue<MwmValue>();
    CHECK(value != nullptr, ("Country file:", m_numMwmIds->GetFile(numMwmId)));

    auto const reader =
        make_unique<FilesContainerR::TReader>(value->m_cont.GetReader(CROSS_MWM_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(*reader);
    auto const p = m_connectors.emplace(numMwmId, CrossMwmConnector(numMwmId));
    fn(VehicleType::Car, p.first->second, src);
    return p.first->second;
  }

  Index & m_index;
  std::shared_ptr<NumMwmIds> m_numMwmIds;

  /// \note |m_connectors| contains cache with transition segments and leap edges.
  /// Each mwm in |m_connectors| may be in two conditions:
  /// * with loaded transition segments (after a call to
  /// CrossMwmConnectorSerializer::DeserializeTransitions())
  /// * with loaded transition segments and with loaded weights
  ///   (after a call to CrossMwmConnectorSerializer::DeserializeTransitions()
  ///   and CrossMwmConnectorSerializer::DeserializeWeights())
  std::map<NumMwmId, CrossMwmConnector> m_connectors;
};
}  // namespace routing
