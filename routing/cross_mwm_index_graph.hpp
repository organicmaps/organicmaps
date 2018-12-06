#pragma once

#include "routing/cross_mwm_connector.hpp"
#include "routing/cross_mwm_connector_serialization.hpp"
#include "routing/fake_feature_ids.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "geometry/point2d.hpp"

#include "coding/file_container.hpp"
#include "coding/point_coding.hpp"
#include "coding/reader.hpp"

#include "indexer/data_source.hpp"

#include "base/logging.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace routing
{
namespace connector
{
template <typename CrossMwmId>
inline FilesContainerR::TReader GetReader(FilesContainerR const & cont)
{
  return cont.GetReader(CROSS_MWM_FILE_TAG);
}

template <>
inline FilesContainerR::TReader GetReader<TransitId>(FilesContainerR const & cont)
{
  return cont.GetReader(TRANSIT_CROSS_MWM_FILE_TAG);
}

template <typename CrossMwmId>
uint32_t constexpr GetFeaturesOffset() noexcept
{
  return 0;
}

template <>
uint32_t constexpr GetFeaturesOffset<TransitId>() noexcept
{
  return FakeFeatureIds::kTransitGraphFeaturesStart;
}

template <typename CrossMwmId>
void AssertConnectorIsFound(NumMwmId neighbor, bool isConnectorFound)
{
  CHECK(isConnectorFound, ("Connector for mwm with number mwm id", neighbor, "was not deserialized."));
}

template <>
inline void AssertConnectorIsFound<TransitId>(NumMwmId /* neighbor */, bool /* isConnectorFound */)
{
}
}  // namespace connector

template <typename CrossMwmId>
class CrossMwmIndexGraph final
{
public:
  using ReaderSourceFile = ReaderSource<FilesContainerR::TReader>;

  CrossMwmIndexGraph(DataSource & dataSource, std::shared_ptr<NumMwmIds> numMwmIds,
                     VehicleType vehicleType)
    : m_dataSource(dataSource), m_numMwmIds(numMwmIds), m_vehicleType(vehicleType)
  {
  }

  bool IsTransition(Segment const & s, bool isOutgoing)
  {
    CrossMwmConnector<CrossMwmId> const & c = GetCrossMwmConnectorWithTransitions(s.GetMwmId());
    return c.IsTransition(s, isOutgoing);
  }

  /// \brief Fills |twins| based on transitions defined in cross_mwm section.
  /// \note In cross_mwm section transitions are defined by osm ids of theirs features.
  /// \note This method fills |twins| with all available twins iff all neighboring of mwm of |s|
  //        have cross_mwm section.
  void GetTwinsByCrossMwmId(Segment const & s, bool isOutgoing, std::vector<NumMwmId> const & neighbors,
                            std::vector<Segment> & twins)
  {
    auto const & crossMwmId = GetCrossMwmConnectorWithTransitions(s.GetMwmId()).GetCrossMwmId(s);

    for (NumMwmId const neighbor : neighbors)
    {
      auto const it = m_connectors.find(neighbor);
      // In case of TransitId, a connector for a mwm with number id |neighbor| may not be found
      // if mwm with such id does not contain corresponding transit_cross_mwm section.
      // It may happen in case of obsolete mwms.
      // Note. Actually it is assumed that connectors always must be found for car routing case.
      // That means mwm without cross_mwm section is not supported.
      connector::AssertConnectorIsFound<CrossMwmId>(neighbor, it != m_connectors.cend());
      if (it == m_connectors.cend())
        continue;

      CrossMwmConnector<CrossMwmId> const & connector = it->second;
      // Note. Last parameter in the method below (isEnter) should be set to |isOutgoing|.
      // If |isOutgoing| == true |s| should be an exit transition segment and the method below searches enters
      // and the last parameter (|isEnter|) should be set to true.
      // If |isOutgoing| == false |s| should be an enter transition segment and the method below searches exits
      // and the last parameter (|isEnter|) should be set to false.
      Segment const * twinSeg = connector.GetTransition(crossMwmId, s.GetSegmentIdx(), isOutgoing);
      if (twinSeg == nullptr)
        continue;

      CHECK_NOT_EQUAL(twinSeg->GetMwmId(), s.GetMwmId(), ());

      // Checks twins for equality.
      // There are same in common, but in case of different version of mwms
      // their's geometry can differ from each other. Because of this we can not
      // build the route, because we fail in astar_algorithm.hpp CHECK(invariant) sometimes.
      if (s.IsRealSegment() || SegmentsAreEqualByGeometry(s, *twinSeg))
        twins.push_back(*twinSeg);
      else
        LOG(LINFO, ("Bad cross mwm feature, differ in geometry. Current:", s, ", twin:", *twinSeg));
    }
  }

  void GetOutgoingEdgeList(Segment const & s, std::vector<SegmentEdge> & edges)
  {
    CrossMwmConnector<CrossMwmId> const & c = GetCrossMwmConnectorWithWeights(s.GetMwmId());
    c.GetOutgoingEdgeList(s, edges);
  }

  void Clear() { m_connectors.clear(); }

  bool InCache(NumMwmId numMwmId) const { return m_connectors.count(numMwmId) != 0; }

  CrossMwmConnector<CrossMwmId> const & GetCrossMwmConnectorWithTransitions(NumMwmId numMwmId)
  {
    auto const it = m_connectors.find(numMwmId);
    if (it != m_connectors.cend())
      return it->second;

    return Deserialize(
        numMwmId,
        CrossMwmConnectorSerializer::DeserializeTransitions<ReaderSourceFile, CrossMwmId>);
  }

  void LoadCrossMwmConnectorWithTransitions(NumMwmId numMwmId)
  {
    GetCrossMwmConnectorWithTransitions(numMwmId);
  }

  std::vector<Segment> const & GetTransitions(NumMwmId numMwmId, bool isEnter)
  {
    auto const & connector = GetCrossMwmConnectorWithTransitions(numMwmId);
    return isEnter ? connector.GetEnters() : connector.GetExits();
  }

private:
  std::vector<m2::PointD> GetFeaturePointsBySegment(Segment const & segment)
  {
    std::vector<m2::PointD> geometry;

    auto const & handle = m_dataSource.GetMwmHandleByCountryFile(m_numMwmIds->GetFile(segment.GetMwmId()));
    if (!handle.IsAlive())
      return geometry;

    auto const & mwmId = handle.GetId();

    auto const & featureId = FeatureID(mwmId, segment.GetFeatureId());

    auto const fillGeometry = [&geometry](FeatureType & ftype)
    {
      ftype.ParseGeometry(FeatureType::BEST_GEOMETRY);
      geometry.reserve(ftype.GetPointsCount());
      for (uint32_t i = 0; i < ftype.GetPointsCount(); ++i)
        geometry.emplace_back(ftype.GetPoint(i));
    };

    m_dataSource.ReadFeature(fillGeometry, featureId);

    return geometry;
  }

  /// \brief Checks segment for equality point by point.
   bool SegmentsAreEqualByGeometry(Segment const & one, Segment const & two)
  {
    std::vector<m2::PointD> geometryOne = GetFeaturePointsBySegment(one);
    std::vector<m2::PointD> geometryTwo = GetFeaturePointsBySegment(two);

    if (geometryOne.size() != geometryTwo.size())
      return false;

    for (uint32_t i = 0; i < geometryOne.size(); ++i)
    {
      if (!base::AlmostEqualAbs(geometryOne[i], geometryTwo[i], kMwmPointAccuracy))
        return false;
    }

    return true;
  }

  CrossMwmConnector<CrossMwmId> const & GetCrossMwmConnectorWithWeights(NumMwmId numMwmId)
  {
    auto const & c = GetCrossMwmConnectorWithTransitions(numMwmId);
    if (c.WeightsWereLoaded())
      return c;

    return Deserialize(
        numMwmId, CrossMwmConnectorSerializer::DeserializeWeights<ReaderSourceFile, CrossMwmId>);
  }

  /// \brief Deserializes connectors for an mwm with |numMwmId|.
  /// \param fn is a function implementing deserialization.
  /// \note Each CrossMwmConnector contained in |m_connectors| may be deserialized in two stages.
  /// The first one is transition deserialization and the second is weight deserialization.
  /// Transition deserialization is much faster and used more often.
  template <typename Fn>
  CrossMwmConnector<CrossMwmId> const & Deserialize(NumMwmId numMwmId, Fn && fn)
  {
    MwmSet::MwmHandle handle = m_dataSource.GetMwmHandleByCountryFile(m_numMwmIds->GetFile(numMwmId));
    if (!handle.IsAlive())
      MYTHROW(RoutingException, ("Mwm", m_numMwmIds->GetFile(numMwmId), "cannot be loaded."));

    MwmValue * value = handle.GetValue<MwmValue>();
    CHECK(value != nullptr, ("Country file:", m_numMwmIds->GetFile(numMwmId)));

    FilesContainerR::TReader const reader =
        FilesContainerR::TReader(connector::GetReader<CrossMwmId>(value->m_cont));
    ReaderSourceFile src(reader);
    auto it = m_connectors.find(numMwmId);
    if (it == m_connectors.end())
      it = m_connectors
               .emplace(numMwmId, CrossMwmConnector<CrossMwmId>(
                                      numMwmId, connector::GetFeaturesOffset<CrossMwmId>()))
               .first;

    fn(m_vehicleType, it->second, src);
    return it->second;
  }

  DataSource & m_dataSource;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  VehicleType m_vehicleType;

  /// \note |m_connectors| contains cache with transition segments and leap edges.
  /// Each mwm in |m_connectors| may be in two conditions:
  /// * with loaded transition segments (after a call to
  /// CrossMwmConnectorSerializer::DeserializeTransitions())
  /// * with loaded transition segments and with loaded weights
  ///   (after a call to CrossMwmConnectorSerializer::DeserializeTransitions()
  ///   and CrossMwmConnectorSerializer::DeserializeWeights())
  std::map<NumMwmId, CrossMwmConnector<CrossMwmId>> m_connectors;
};
}  // namespace routing
