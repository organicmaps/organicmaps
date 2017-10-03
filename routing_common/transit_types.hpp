#pragma once

#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <cstdint>
#include <limits>
#include <string>

namespace routing
{
namespace transit
{
using LineId = uint32_t;
using StopId = uint64_t;
using TransferId = uint64_t;
using NetworkId = uint32_t;
using FeatureId = uint32_t;

LineId constexpr kInvalidLineId = std::numeric_limits<LineId>::max();
StopId constexpr kInvalidStopId = std::numeric_limits<StopId>::max();
TransferId constexpr kInvalidTransferId = std::numeric_limits<TransferId>::max();
NetworkId constexpr kInvalidNetworkId = std::numeric_limits<NetworkId>::max();
FeatureId constexpr kInvalidFeatureId = std::numeric_limits<FeatureId>::max();

struct TransitHeader
{
  TransitHeader() { Reset(); }
  TransitHeader(uint16_t version, uint32_t gatesOffset, uint32_t edgesOffset,
                uint32_t transfersOffset, uint32_t linesOffset, uint32_t shapesOffset,
                uint32_t networksOffset, uint32_t endOffset);
  void Reset();
  bool IsEqualForTesting(TransitHeader const & header) const;

  DECLARE_VISITOR(visitor(m_version, "m_version"), visitor(m_reserve, "m_reserve"),
                  visitor(m_gatesOffset, "m_gatesOffset"), visitor(m_edgesOffset, "m_edgesOffset"),
                  visitor(m_transfersOffset, "m_transfersOffset"),
                  visitor(m_linesOffset, "m_linesOffset"),
                  visitor(m_shapesOffset, "m_shapesOffset"),
                  visitor(m_networksOffset, "m_networksOffset"),
                  visitor(m_endOffset, "m_endOffset"))
  DECLARE_DEBUG_PRINT(TransitHeader)

  uint16_t m_version;
  uint16_t m_reserve;
  uint32_t m_gatesOffset;
  uint32_t m_edgesOffset;
  uint32_t m_transfersOffset;
  uint32_t m_linesOffset;
  uint32_t m_shapesOffset;
  uint32_t m_networksOffset;
  uint32_t m_endOffset;
};

static_assert(sizeof(TransitHeader) == 32, "Wrong header size of transit section.");

class Stop
{
public:
  Stop() = default;
  Stop(StopId id, FeatureId featureId, TransferId m_transferId, std::vector<LineId> const & lineIds,
       m2::PointD const & point);
  bool IsEqualForTesting(Stop const & stop) const;

  DECLARE_VISITOR(visitor(m_id, "id"), visitor(m_featureId, "osm_id"),
                  visitor(m_transferId, "transfer_id"), visitor(m_lineIds, "line_ids"),
                  visitor(m_point, "point"))
  DECLARE_DEBUG_PRINT(Stop)

private:
  StopId m_id = kInvalidStopId;
  FeatureId m_featureId = kInvalidFeatureId;
  TransferId m_transferId = kInvalidTransferId;
  std::vector<LineId> m_lineIds;
  m2::PointD m_point;
  // @TODO(bykoianko) It's necessary to add field m_titleAnchors here and implement serialization
  // and deserialization.
};

// @TODO(bykoianko) Data structures and methods for other transit data should be implemented in here.
}  // namespace transit
}  // namespace routing
