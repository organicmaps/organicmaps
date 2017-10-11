#pragma once

#include "routing/cross_mwm_connector.hpp"
#include "routing/segment.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/transition_points.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"
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
  using ReaderSourceFile = ReaderSource<FilesContainerR::TReader>;

  CrossMwmIndexGraph(Index & index, std::shared_ptr<NumMwmIds> numMwmIds, VehicleType vehicleType)
    : m_index(index), m_numMwmIds(numMwmIds), m_vehicleType(vehicleType)
  {
  }

  bool IsTransition(Segment const & s, bool isOutgoing);
  /// \brief Fills |twins| based on transitions defined in cross_mwm section.
  /// \note In cross_mwm section transitions are defined by osm ids of theirs features.
  /// \note This method fills |twins| with all available twins iff all neighboring of mwm of |s|
  //        have cross_mwm section.
  void GetTwinsByOsmId(Segment const & s, bool isOutgoing, std::vector<NumMwmId> const & neighbors,
                       std::vector<Segment> & twins);
  void GetEdgeList(Segment const & s, bool isOutgoing, std::vector<SegmentEdge> & edges);
  void Clear() { m_connectors.clear(); }
  TransitionPoints GetTransitionPoints(Segment const & s, bool isOutgoing);
  bool InCache(NumMwmId numMwmId) const { return m_connectors.count(numMwmId) != 0; }
  CrossMwmConnector const & GetCrossMwmConnectorWithTransitions(NumMwmId numMwmId);

  template <typename Fn>
  void ForEachTransition(NumMwmId numMwmId, bool isEnter, Fn && fn)
  {
    auto const & connectors = GetCrossMwmConnectorWithTransitions(numMwmId);
    for (Segment const & t : (isEnter ? connectors.GetEnters() : connectors.GetExits()))
      fn(t);
  }

private:
  CrossMwmConnector const & GetCrossMwmConnectorWithWeights(NumMwmId numMwmId);

  /// \brief Deserializes connectors for an mwm with |numMwmId|.
  /// \param fn is a function implementing deserialization.
  /// \note Each CrossMwmConnector contained in |m_connectors| may be deserizalize in two stages.
  /// The first one is transition deserialization and the second is weight deserialization.
  /// Transition deserialization is much faster and used more often.
  template <typename Fn>
  CrossMwmConnector const & Deserialize(NumMwmId numMwmId, Fn && fn)
  {
    MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(m_numMwmIds->GetFile(numMwmId));
    if (!handle.IsAlive())
      MYTHROW(RoutingException, ("Mwm", m_numMwmIds->GetFile(numMwmId), "cannot be loaded."));

    MwmValue * value = handle.GetValue<MwmValue>();
    CHECK(value != nullptr, ("Country file:", m_numMwmIds->GetFile(numMwmId)));

    FilesContainerR::TReader const reader =
        FilesContainerR::TReader(value->m_cont.GetReader(CROSS_MWM_FILE_TAG));
    ReaderSourceFile src(reader);
    auto it = m_connectors.find(numMwmId);
    if (it == m_connectors.end())
      it = m_connectors.emplace(numMwmId, CrossMwmConnector(numMwmId)).first;

    fn(m_vehicleType, it->second, src);
    return it->second;
  }

  Index & m_index;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  VehicleType m_vehicleType;

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
