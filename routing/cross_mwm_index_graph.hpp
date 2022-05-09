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

#include "coding/files_container.hpp"
#include "coding/point_coding.hpp"
#include "coding/reader.hpp"

#include "indexer/data_source.hpp"

#include "base/logging.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <type_traits>
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

  template <class FnT> void ForEachTransitSegmentId(NumMwmId numMwmId, uint32_t featureId, FnT && fn)
  {
    GetCrossMwmConnectorWithTransitions(numMwmId).ForEachTransitSegmentId(featureId, fn);
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
      auto const twinSeg = connector.GetTransition(crossMwmId, s.GetSegmentIdx(), isOutgoing);
      if (!twinSeg)
        continue;

      // Twins should have the same direction, because we assume that twins are the same segments
      // with one difference - they are in the different mwms. Twins could have not same direction
      // in case of different mwms versions. For example |s| is cross mwm Enter from elder version
      // and somebody moved points of ways such that |twinSeg| became cross mwm Exit from newer
      // version. Because of that |twinSeg.IsForward()| will differ from |s.IsForward()|.
      if (twinSeg->IsForward() != s.IsForward())
        continue;

      CHECK_NOT_EQUAL(twinSeg->GetMwmId(), s.GetMwmId(), ());

      // Checks twins for equality if they are from different mwm versions.
      // There are same in common, but in case of different version of mwms
      // their's geometry can differ from each other. Because of this we can not
      // build the route, because we fail in astar_algorithm.hpp CHECK(invariant) sometimes.
      auto const & sMwmId = m_dataSource.GetMwmIdByCountryFile(m_numMwmIds->GetFile(s.GetMwmId()));
      CHECK(sMwmId.IsAlive(), (s));
      auto const & twinSegMwmId = m_dataSource.GetMwmIdByCountryFile(m_numMwmIds->GetFile(twinSeg->GetMwmId()));
      CHECK(twinSegMwmId.IsAlive(), (*twinSeg));

      if (sMwmId.GetInfo()->GetVersion() == twinSegMwmId.GetInfo()->GetVersion() ||
          SegmentsAreEqualByGeometry(s, *twinSeg))
      {
        twins.push_back(*twinSeg);
      }
      else
      {
        LOG(LINFO, ("Bad cross mwm feature, differ in geometry. Current:", s, ", twin:", *twinSeg));
      }
    }
  }

  using EdgeListT = typename CrossMwmConnector<CrossMwmId>::EdgeListT;

  void GetOutgoingEdgeList(Segment const & s, EdgeListT & edges)
  {
    CrossMwmConnector<CrossMwmId> const & c = GetCrossMwmConnectorWithWeights(s.GetMwmId());
    c.GetOutgoingEdgeList(s, edges);
  }

  void GetIngoingEdgeList(Segment const & s, EdgeListT & edges)
  {
    CrossMwmConnector<CrossMwmId> const & c = GetCrossMwmConnectorWithWeights(s.GetMwmId());
    c.GetIngoingEdgeList(s, edges);
  }

//  void Clear()
//  {
//    m_connectors.clear();
//  }
  void Purge()
  {
    ConnectersMapT tmp;
    tmp.swap(m_connectors);
  }

  bool InCache(NumMwmId numMwmId) const { return m_connectors.count(numMwmId) != 0; }

  CrossMwmConnector<CrossMwmId> const & GetCrossMwmConnectorWithTransitions(NumMwmId numMwmId)
  {
    auto const it = m_connectors.find(numMwmId);
    if (it != m_connectors.cend())
      return it->second;

    return Deserialize(numMwmId, [this](CrossMwmConnectorBuilder<CrossMwmId> & builder, auto & src)
    {
      builder.DeserializeTransitions(m_vehicleType, src);
    });
  }

  void LoadCrossMwmConnectorWithTransitions(NumMwmId numMwmId)
  {
    GetCrossMwmConnectorWithTransitions(numMwmId);
  }

  template <class FnT> void ForEachTransition(NumMwmId numMwmId, bool isEnter, FnT && fn)
  {
    auto const & connector = GetCrossMwmConnectorWithTransitions(numMwmId);

    auto const wrapper = [&fn](uint32_t, Segment const & s) { fn(s); };
    return isEnter ? connector.ForEachEnter(wrapper) : connector.ForEachExit(wrapper);
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
    // Do not check for transit graph.
    if (!one.IsRealSegment() || !two.IsRealSegment())
      return true;

    static_assert(std::is_same<CrossMwmId, base::GeoObjectId>::value ||
                  std::is_same<CrossMwmId, connector::TransitId>::value,
                  "Be careful of usage other ids here. "
                  "Make sure, there is not crash with your new CrossMwmId");

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

    return Deserialize(numMwmId,  [](CrossMwmConnectorBuilder<CrossMwmId> & builder, auto & src)
    {
      builder.DeserializeWeights(src);
    });
  }

  /// \brief Deserializes connectors for an mwm with |numMwmId|.
  /// \param fn is a function implementing deserialization.
  /// \note Each CrossMwmConnector contained in |m_connectors| may be deserialized in two stages.
  /// The first one is transition deserialization and the second is weight deserialization.
  /// Transition deserialization is much faster and used more often.
  template <typename Fn>
  CrossMwmConnector<CrossMwmId> const & Deserialize(NumMwmId numMwmId, Fn && fn)
  {
    auto const & file = m_numMwmIds->GetFile(numMwmId);
    MwmSet::MwmHandle handle = m_dataSource.GetMwmHandleByCountryFile(file);
    if (!handle.IsAlive())
      MYTHROW(RoutingException, ("Mwm", file, "cannot be loaded."));

    MwmValue const * value = handle.GetValue();
    CHECK(value != nullptr, ("Country file:", file));

    auto it = m_connectors.emplace(numMwmId, CrossMwmConnector<CrossMwmId>(numMwmId)).first;

    CrossMwmConnectorBuilder<CrossMwmId> builder(it->second);
    builder.ApplyNumerationOffset();

    auto reader = connector::GetReader<CrossMwmId>(value->m_cont);
    fn(builder, reader);
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
  using ConnectersMapT = std::map<NumMwmId, CrossMwmConnector<CrossMwmId>>;
  ConnectersMapT m_connectors;
};
}  // namespace routing
