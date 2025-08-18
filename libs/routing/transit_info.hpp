#pragma once

#include "transit/experimental/transit_types_experimental.hpp"
#include "transit/transit_entities.hpp"
#include "transit/transit_types.hpp"
#include "transit/transit_version.hpp"

#include "indexer/feature_decl.hpp"

#include "base/assert.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace routing
{
class TransitInfo final
{
public:
  enum class Type
  {
    Gate,
    Edge,
    Transfer
  };

  struct EdgeSubway
  {
    EdgeSubway() = default;
    explicit EdgeSubway(transit::Edge const & edge)
      : m_lineId(edge.GetLineId())
      , m_stop1Id(edge.GetStop1Id())
      , m_stop2Id(edge.GetStop2Id())
      , m_shapeIds(edge.GetShapeIds())
    {
      ASSERT(!edge.GetTransfer(), ());
    }

    transit::LineId m_lineId = transit::kInvalidLineId;
    transit::StopId m_stop1Id = transit::kInvalidStopId;
    transit::StopId m_stop2Id = transit::kInvalidStopId;
    std::vector<transit::ShapeId> m_shapeIds;
  };

  struct EdgePT
  {
    EdgePT() = default;
    explicit EdgePT(::transit::experimental::Edge const & edge)
      : m_lineId(edge.GetLineId())
      , m_stop1Id(edge.GetStop1Id())
      , m_stop2Id(edge.GetStop2Id())
      , m_shapeLink(edge.GetShapeLink())
    {
      ASSERT(!edge.IsTransfer(), ());
    }

    ::transit::TransitId m_lineId = ::transit::kInvalidTransitId;
    ::transit::TransitId m_stop1Id = ::transit::kInvalidTransitId;
    ::transit::TransitId m_stop2Id = ::transit::kInvalidTransitId;
    ::transit::ShapeLink m_shapeLink;
  };

  struct GateSubway
  {
    GateSubway() = default;
    explicit GateSubway(transit::Gate const & gate) : m_featureId(gate.GetFeatureId()) {}

    transit::FeatureId m_featureId = kInvalidFeatureId;
  };

  struct GatePT
  {
    GatePT() = default;
    explicit GatePT(::transit::experimental::Gate const & gate) : m_featureId(gate.GetFeatureId()) {}

    ::transit::experimental::FeatureId m_featureId = kInvalidFeatureId;
  };

  struct TransferSubway
  {
    TransferSubway() = default;
    explicit TransferSubway(transit::Edge const & edge) : m_stop1Id(edge.GetStop1Id()), m_stop2Id(edge.GetStop2Id())
    {
      ASSERT(edge.GetTransfer(), ());
    }

    transit::StopId m_stop1Id = transit::kInvalidStopId;
    transit::StopId m_stop2Id = transit::kInvalidStopId;
  };

  struct TransferPT
  {
    TransferPT() = default;
    explicit TransferPT(::transit::experimental::Edge const & edge)
      : m_stop1Id(edge.GetStop1Id())
      , m_stop2Id(edge.GetStop2Id())
    {
      ASSERT(edge.IsTransfer(), ());
    }

    ::transit::TransitId m_stop1Id = ::transit::kInvalidTransitId;
    ::transit::TransitId m_stop2Id = ::transit::kInvalidTransitId;
  };

  explicit TransitInfo(transit::Gate const & gate)
    : m_transitVersion(::transit::TransitVersion::OnlySubway)
    , m_type(Type::Gate)
    , m_gateSubway(gate)
  {}

  explicit TransitInfo(::transit::experimental::Gate const & gate)
    : m_transitVersion(::transit::TransitVersion::AllPublicTransport)
    , m_type(Type::Gate)
    , m_gatePT(gate)
  {}

  explicit TransitInfo(transit::Edge const & edge)
    : m_transitVersion(::transit::TransitVersion::OnlySubway)
    , m_type(edge.GetTransfer() ? Type::Transfer : Type::Edge)
    , m_edgeSubway(edge.GetTransfer() ? EdgeSubway() : EdgeSubway(edge))
    , m_transferSubway(edge.GetTransfer() ? TransferSubway(edge) : TransferSubway())
  {}

  explicit TransitInfo(::transit::experimental::Edge const & edge)
    : m_transitVersion(::transit::TransitVersion::AllPublicTransport)
    , m_type(edge.IsTransfer() ? Type::Transfer : Type::Edge)
    , m_edgePT(edge.IsTransfer() ? EdgePT() : EdgePT(edge))
    , m_transferPT(edge.IsTransfer() ? TransferPT(edge) : TransferPT())
  {}

  Type GetType() const { return m_type; }

  ::transit::TransitVersion GetVersion() const { return m_transitVersion; }

  EdgeSubway const & GetEdgeSubway() const
  {
    ASSERT_EQUAL(m_type, Type::Edge, ());
    CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::OnlySubway, ());
    return m_edgeSubway;
  }

  GateSubway const & GetGateSubway() const
  {
    ASSERT_EQUAL(m_type, Type::Gate, ());
    return m_gateSubway;
  }

  TransferSubway const & GetTransferSubway() const
  {
    ASSERT_EQUAL(m_type, Type::Transfer, ());
    return m_transferSubway;
  }

  EdgePT const & GetEdgePT() const
  {
    ASSERT_EQUAL(m_type, Type::Edge, ());
    CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::AllPublicTransport, ());
    return m_edgePT;
  }

  GatePT const & GetGatePT() const
  {
    CHECK_EQUAL(m_type, Type::Gate, ());
    CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::AllPublicTransport, ());
    return m_gatePT;
  }

  TransferPT const & GetTransferPT() const
  {
    ASSERT_EQUAL(m_type, Type::Transfer, ());
    CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::AllPublicTransport, ());
    return m_transferPT;
  }

private:
  ::transit::TransitVersion const m_transitVersion;
  Type const m_type;
  // Valid for m_type == Type::Edge only.
  EdgeSubway const m_edgeSubway;
  // Valid for m_type == Type::Gate only.
  GateSubway const m_gateSubway;
  // Valid for m_type == Type::Transfer only.
  TransferSubway const m_transferSubway;

  EdgePT const m_edgePT;
  GatePT const m_gatePT;
  TransferPT const m_transferPT;
};

class TransitInfoWrapper final
{
public:
  explicit TransitInfoWrapper(std::unique_ptr<TransitInfo> ptr) : m_ptr(std::move(ptr)) {}
  explicit TransitInfoWrapper(TransitInfoWrapper && rhs) { swap(m_ptr, rhs.m_ptr); }
  explicit TransitInfoWrapper(TransitInfoWrapper const & rhs)
  {
    if (rhs.m_ptr)
      m_ptr = std::make_unique<TransitInfo>(*rhs.m_ptr);
  }

  TransitInfoWrapper & operator=(TransitInfoWrapper && rhs)
  {
    swap(m_ptr, rhs.m_ptr);
    return *this;
  }

  TransitInfoWrapper & operator=(TransitInfoWrapper const & rhs)
  {
    if (this == &rhs)
      return *this;

    m_ptr.reset();
    if (rhs.m_ptr)
      m_ptr = std::make_unique<TransitInfo>(*rhs.m_ptr);
    return *this;
  }

  bool HasTransitInfo() const { return m_ptr != nullptr; }

  TransitInfo const & Get() const
  {
    ASSERT(HasTransitInfo(), ());
    return *m_ptr;
  }

  void Set(std::unique_ptr<TransitInfo> ptr) { m_ptr = std::move(ptr); }

private:
  std::unique_ptr<TransitInfo> m_ptr;
};

inline std::string DebugPrint(TransitInfo::Type type)
{
  switch (type)
  {
  case TransitInfo::Type::Gate: return "Gate";
  case TransitInfo::Type::Edge: return "Edge";
  case TransitInfo::Type::Transfer: return "Transfer";
  }
  UNREACHABLE();
}
}  // namespace routing
