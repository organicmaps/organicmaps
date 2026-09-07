#pragma once

#include "transit/transit_types.hpp"

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

  struct GateSubway
  {
    GateSubway() = default;
    explicit GateSubway(transit::Gate const & gate) : m_featureId(gate.GetFeatureId()) {}

    transit::FeatureId m_featureId = kInvalidFeatureId;
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

  explicit TransitInfo(transit::Gate const & gate) : m_type(Type::Gate), m_gateSubway(gate) {}

  explicit TransitInfo(transit::Edge const & edge)
    : m_type(edge.GetTransfer() ? Type::Transfer : Type::Edge)
    , m_edgeSubway(edge.GetTransfer() ? EdgeSubway() : EdgeSubway(edge))
    , m_transferSubway(edge.GetTransfer() ? TransferSubway(edge) : TransferSubway())
  {}

  Type GetType() const { return m_type; }

  EdgeSubway const & GetEdgeSubway() const
  {
    ASSERT_EQUAL(m_type, Type::Edge, ());
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

private:
  Type const m_type;
  // Valid for m_type == Type::Edge only.
  EdgeSubway const m_edgeSubway;
  // Valid for m_type == Type::Gate only.
  GateSubway const m_gateSubway;
  // Valid for m_type == Type::Transfer only.
  TransferSubway const m_transferSubway;
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
